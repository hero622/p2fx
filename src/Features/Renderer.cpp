#include "Renderer.hpp"

#include "Command.hpp"
#include "Event.hpp"
#include "Hook.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Features/Session.hpp"
#include "Utils/SDK.hpp"
#include "Utils/Math.hpp"

// Stuff pulled from the engine
static void **g_videomode;
static int *g_snd_linear_count;
static int **g_snd_p;
static int *g_snd_vol;
static MovieInfo_t *g_movieInfo;

// The demoplayer tick this segment ends on (we'll stop recording at
// this tick if p2fx_render_autostop is set)
int Renderer::segmentEndTick = -1;

// Whether a demo is loading; used to detect whether we should autostart
bool Renderer::isDemoLoading = false;

// We reference this so we can make sure the output is stereo.
// Supporting other output options is quite tricky; because Source
// movies only support stereo output, the functions for outputting
// surround audio don't have the necessary code for writing it to
// buffers like we need. This is *possible* to get around, but would
// require some horrible mid-function detours.
static Variable snd_surround_speakers;

// h264, hevc, vp8, vp9, dnxhd
// aac, ac3, vorbis, opus, flac
static Variable p2fx_render_vbitrate("p2fx_render_vbitrate", "40000", 1, "Video bitrate used in renders (kbit/s)\n");
static Variable p2fx_render_abitrate("p2fx_render_abitrate", "160", 1, "Audio bitrate used in renders (kbit/s)\n");
static Variable p2fx_render_vcodec("p2fx_render_vcodec", "h264", "Video codec used in renders (h264, hevc, vp8, vp9, dnxhd)\n", 0);
static Variable p2fx_render_acodec("p2fx_render_acodec", "aac", "Audio codec used in renders (aac, ac3, vorbis, opus, flac)\n", 0);
static Variable p2fx_render_quality("p2fx_render_quality", "35", 0, 50, "Render output quality, higher is better (50=lossless)\n");
static Variable p2fx_render_fps("p2fx_render_fps", "60", 1, "Render output FPS\n");
static Variable p2fx_render_sample_rate("p2fx_render_sample_rate", "44100", 1000, "Audio output sample rate\n");
static Variable p2fx_render_blend("p2fx_render_blend", "1", 0, "How many frames to blend for each output frame; 1 = do not blend, 0 = automatically determine based on host_framerate\n");
static Variable p2fx_render_blend_mode("p2fx_render_blend_mode", "1", 0, 1, "What type of frameblending to use. 0 = linear, 1 = Gaussian\n");
static Variable p2fx_render_autostart("p2fx_render_autostart", "0", "Whether to automatically start when demo playback begins\n");
static Variable p2fx_render_autostart_extension("p2fx_render_autostart_extension", "avi", "The file extension (and hence container format) to use for automatically started renders.\n", 0);
static Variable p2fx_render_autostop("p2fx_render_autostop", "1", "Whether to automatically stop when __END__ is seen in demo playback\n");
static Variable p2fx_render_shutter_angle("p2fx_render_shutter_angle", "360", 30, 360, "The shutter angle to use for rendering in degrees.\n");
static Variable p2fx_render_merge("p2fx_render_merge", "0", "When set, merge all the renders until p2fx_render_finish is entered\n");
static Variable p2fx_render_skip_coop_videos("p2fx_render_skip_coop_videos", "1", "When set, don't include coop loading time in renders\n");

// g_videomode VMT wrappers {{{

static inline int GetScreenWidth() {
	return Memory::VMT<int(__rescall *)(void *)>(*g_videomode, Offsets::GetModeWidth)(*g_videomode);
}

static inline int GetScreenHeight() {
	return Memory::VMT<int(__rescall *)(void *)>(*g_videomode, Offsets::GetModeHeight)(*g_videomode);
}

static inline void ReadScreenPixels(int x, int y, int w, int h, void *buf, ImageFormat fmt) {
	return Memory::VMT<void(__rescall *)(void *, int, int, int, int, void *, ImageFormat)>(*g_videomode, Offsets::ReadScreenPixels)(*g_videomode, x, y, w, h, buf, fmt);
}

// }}}

ON_EVENT(P2FX_UNLOAD) {
	if (Renderer::g_render.worker.joinable()) Renderer::g_render.worker.detach();
}

static inline void msgStopRender(bool error) {
	std::lock_guard<std::mutex> lock(Renderer::g_render.workerUpdateLock);
	Renderer::g_render.workerMsg.store(error ? Renderer::WorkerMsg::STOP_RENDERING_ERROR : Renderer::WorkerMsg::STOP_RENDERING_REQUESTED);
	Renderer::g_render.workerUpdate.notify_all();
}

// Utilities {{{

static AVCodecID videoCodecFromName(const char *name) {
	if (!strcmp(name, "h264")) return AV_CODEC_ID_H264;
	if (!strcmp(name, "hevc")) return AV_CODEC_ID_HEVC;
	if (!strcmp(name, "vp8")) return AV_CODEC_ID_VP8;
	if (!strcmp(name, "vp9")) return AV_CODEC_ID_VP9;
	if (!strcmp(name, "dnxhd")) return AV_CODEC_ID_DNXHD;
	return AV_CODEC_ID_NONE;
}

static AVCodecID audioCodecFromName(const char *name) {
	if (!strcmp(name, "aac")) return AV_CODEC_ID_AAC;
	if (!strcmp(name, "ac3")) return AV_CODEC_ID_AC3;
	if (!strcmp(name, "vorbis")) return AV_CODEC_ID_VORBIS;
	if (!strcmp(name, "opus")) return AV_CODEC_ID_OPUS;
	if (!strcmp(name, "flac")) return AV_CODEC_ID_FLAC;
	return AV_CODEC_ID_NONE;
}

static uint16_t calcFrameWeight(double position) {
	int mode = p2fx_render_blend_mode.GetInt();
	if (mode == 0) {
		// Linear
		return 1;
	} else if (mode == 1) {
		// Gaussian

		const double mean = 0.0;
		const double stdDev = 1.0;

		const double variance = stdDev*stdDev;

		// I'm pretty sure srcdemo2's formula here is wrong. This is a
		// *correct* Gaussian weighting!

		double x = position * 2.0 - 1.0; // rescale to [-1,1]
		double w = 1.0/(stdDev * sqrt(M_PI)) * exp(-(((x - mean) * (x - mean)) / (2 * variance)));
		return (uint16_t)(w * 16000.0); // theoretically 16384 but this protects against dodgy precision stuff
	}

	return 1;
}

// }}}

// Movie command hooks {{{

// We want to stop use of the normal movie system while a P2FX render is
// active. This is because we exploit g_movieInfo to make the game think
// it's recording a movie, so if both were running at the same time,
// they'd interact in weird and undefined ways.

static _CommandCallback startmovie_origCbk;
static _CommandCallback endmovie_origCbk;
static void startmovie_cbk(const CCommand &args) {
	if (Renderer::g_render.isRendering.load()) {
		console->Print("Cannot startmovie while a P2FX render is active! Stop the render with p2fx_render_finish.\n");
		return;
	}
	startmovie_origCbk(args);
}
static void endmovie_cbk(const CCommand &args) {
	if (Renderer::g_render.isRendering.load()) {
		console->Print("Cannot endmovie while a P2FX render is active! Did you mean p2fx_render_finish?\n");
		return;
	}
	endmovie_origCbk(args);
}

// }}}

// AVFrame allocators {{{

static AVFrame *allocPicture(AVPixelFormat pixFmt, int width, int height) {
	AVFrame *picture = av_frame_alloc();
	if (!picture) {
		return NULL;
	}

	picture->format = pixFmt;
	picture->width = width;
	picture->height = height;

	if (av_frame_get_buffer(picture, 32) < 0) {
		console->Print("Failed to allocate frame data\n");
		av_frame_free(&picture);
		return NULL;
	}

	return picture;
}

static AVFrame *allocAudioFrame(AVSampleFormat sampleFmt, uint64_t channelLayout, int sampleRate, int nbSamples) {
	AVFrame *frame = av_frame_alloc();
	if (!frame) {
		return NULL;
	}

	frame->format = sampleFmt;
	frame->channel_layout = channelLayout;
	frame->sample_rate = sampleRate;
	frame->nb_samples = nbSamples;

	if (nbSamples) {
		if (av_frame_get_buffer(frame, 0) < 0) {
			return NULL;
		}
	}

	return frame;
}

// }}}

// Renderer::Stream creation and destruction {{{

// framerate is sample rate for audio streams
static bool addStream(Renderer::Stream *out, AVFormatContext *outputCtx, AVCodecID codecId, int64_t bitrate, int framerate, int ptsOff, int width = 0, int height = 0) {
	out->codec = avcodec_find_encoder(codecId);
	if (!out->codec) {
		console->Print("Failed to find encoder for '%s'!\n", avcodec_get_name(codecId));
		return false;
	}

	// dnxhd bitrate selection {{{

	if (codecId == AV_CODEC_ID_DNXHD) {
		// dnxhd is fussy and won't just allow any bitrate or
		// resolution, so check our resolution is supported and find the
		// closest bitrate to what was requested

		// rates here are in Mbps
		int64_t *rates;
		size_t nrates;

		if (width == 1920 && height == 1080) {  // 1080p 16:9
			static int64_t rates1080[] = {
				36,
				45,
				75,
				90,
				115,
				120,
				145,
				175,
				180,
				190,
				220,
				240,
				365,
				440,
			};
			rates = rates1080;
			nrates = sizeof rates1080 / sizeof rates1080[0];
		} else if (width == 1280 && height == 720) {  // 720p 16:9
			static int64_t rates720[] = {
				60,
				75,
				90,
				110,
				120,
				145,
				180,
				220,
			};
			rates = rates720;
			nrates = sizeof rates720 / sizeof rates720[0];
		} else if (width == 1440 && height == 1080) {  // 1080p 4:3
			static int64_t rates1080[] = {
				63,
				84,
				100,
				110,
			};
			rates = rates1080;
			nrates = sizeof rates1080 / sizeof rates1080[0];
		} else if (width == 960 && height == 720) {  // 720p 4:3
			static int64_t rates720[] = {
				42,
				60,
				75,
				115,
			};
			rates = rates720;
			nrates = sizeof rates720 / sizeof rates720[0];
		} else {
			console->Print("Resolution not supported by dnxhd\n");
			return false;
		}

		int64_t realBitrate = -1;
		int64_t lastDelta = INT64_MAX;

		for (size_t i = 0; i < nrates; ++i) {
			int64_t rate = rates[i] * 1000000;
			int64_t delta = rate > bitrate ? rate - bitrate : bitrate - rate;
			if (delta < lastDelta) {
				realBitrate = rate;
			}
		}

		if (realBitrate != bitrate) {
			console->Print("dnxhd does not support the given bitrate; encoding at %d kb/s instead\n", realBitrate / 1000);
			bitrate = realBitrate;
		}
	}

	// }}}

	out->stream = avformat_new_stream(outputCtx, NULL);
	if (!out->stream) {
		console->Print("Failed to allocate stream\n");
		return false;
	}

	out->enc = avcodec_alloc_context3(out->codec);
	if (!out->enc) {
		console->Print("Failed to allocate an encoding context\n");
		return false;
	}

	out->enc->bit_rate = bitrate;
	out->stream->time_base = {1, framerate};

	switch (out->codec->type) {
	case AVMEDIA_TYPE_AUDIO:
		out->enc->sample_fmt = out->codec->sample_fmts ? out->codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;  // don't really care about sample format
		out->enc->sample_rate = framerate;
		out->enc->channel_layout = AV_CH_LAYOUT_STEREO;
		out->enc->channels = av_get_channel_layout_nb_channels(out->enc->channel_layout);

		if (out->codec->channel_layouts) {
			bool foundStereo = false;
			for (const uint64_t *layout = out->codec->channel_layouts; *layout; ++layout) {
				if (*layout == AV_CH_LAYOUT_STEREO) {
					foundStereo = true;
					break;
				}
			}

			if (!foundStereo) {
				console->Print("Stereo not supported by audio encoder\n");
				avcodec_free_context(&out->enc);
				return false;
			}
		}

		if (out->codec->supported_samplerates) {
			bool foundRate = false;
			for (const int *rate = out->codec->supported_samplerates; *rate; ++rate) {
				if (*rate == framerate) {
					foundRate = true;
					break;
				}
			}

			if (!foundRate) {
				console->Print("Sample rate %d not supported by audio encoder\n", framerate);
				avcodec_free_context(&out->enc);
				return false;
			}
		}

		break;

	case AVMEDIA_TYPE_VIDEO:
		out->enc->codec_id = codecId;
		out->enc->width = width;
		out->enc->height = height;
		out->enc->time_base = out->stream->time_base;
		out->enc->gop_size = 12;
		out->enc->pix_fmt = codecId == AV_CODEC_ID_DNXHD ? AV_PIX_FMT_YUV422P : AV_PIX_FMT_YUV420P;

		break;

	default:
		break;
	}

	if (outputCtx->oformat->flags & AVFMT_GLOBALHEADER) {
		out->enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	out->nextPts = ptsOff;

	return true;
}

static void closeStream(Renderer::Stream *s) {
	avcodec_free_context(&s->enc);
	av_frame_free(&s->frame);
	av_frame_free(&s->tmpFrame);
	sws_freeContext(s->swsCtx);
	swr_free(&s->swrCtx);
}

// }}}

// Flushing streams {{{

static bool flushStream(Renderer::Stream *s, bool isEnd = false) {
	if (isEnd) {
		avcodec_send_frame(s->enc, NULL);
	}

	while (true) {
		AVPacket pkt = {0};
		int ret = avcodec_receive_packet(s->enc, &pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return true;
		} else if (ret < 0) {
			return false;
		}
		av_packet_rescale_ts(&pkt, s->enc->time_base, s->stream->time_base);
		pkt.stream_index = s->stream->index;
		av_interleaved_write_frame(Renderer::g_render.outCtx, &pkt);
		av_packet_unref(&pkt);
	}
}

// }}}

// Opening streams {{{

static bool openVideo(AVFormatContext *outputCtx, Renderer::Stream *s, AVDictionary **options) {
	if (avcodec_open2(s->enc, s->codec, options) < 0) {
		console->Print("Failed to open video codec\n");
		return false;
	}

	s->frame = allocPicture(s->enc->pix_fmt, s->enc->width, s->enc->height);
	if (!s->frame) {
		console->Print("Failed to allocate video frame\n");
		return false;
	}

	s->tmpFrame = allocPicture(AV_PIX_FMT_BGR24, s->enc->width, s->enc->height);
	if (!s->tmpFrame) {
		console->Print("Failed to allocate intermediate video frame\n");
		return false;
	}

	if (avcodec_parameters_from_context(s->stream->codecpar, s->enc) < 0) {
		console->Print("Failed to copy stream parameters\n");
		return false;
	}

	s->swsCtx = sws_getContext(s->enc->width, s->enc->height, AV_PIX_FMT_BGR24, s->enc->width, s->enc->height, s->enc->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
	if (!s->swsCtx) {
		console->Print("Failed to initialize conversion context\n");
		return false;
	}

	return true;
}

static bool openAudio(AVFormatContext *outputCtx, Renderer::Stream *s, AVDictionary **options) {
	if (avcodec_open2(s->enc, s->codec, options) < 0) {
		console->Print("Failed to open audio codec\n");
		return false;
	}

	s->frame = allocAudioFrame(s->enc->sample_fmt, s->enc->channel_layout, s->enc->sample_rate, s->enc->frame_size);
	s->tmpFrame = allocAudioFrame(AV_SAMPLE_FMT_S16P, s->enc->channel_layout, s->enc->sample_rate, s->enc->frame_size);

	if (avcodec_parameters_from_context(s->stream->codecpar, s->enc) < 0) {
		console->Print("Failed to copy stream parameters\n");
		return false;
	}

	s->swrCtx = swr_alloc();
	if (!s->swrCtx) {
		console->Print("Failed to allocate resampler context\n");
		return false;
	}

	av_opt_set_int(s->swrCtx, "in_channel_count", s->enc->channels, 0);
	av_opt_set_int(s->swrCtx, "in_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(s->swrCtx, "in_sample_fmt", AV_SAMPLE_FMT_S16P, 0);
	av_opt_set_int(s->swrCtx, "out_channel_count", s->enc->channels, 0);
	av_opt_set_int(s->swrCtx, "out_sample_rate", s->enc->sample_rate, 0);
	av_opt_set_sample_fmt(s->swrCtx, "out_sample_fmt", s->enc->sample_fmt, 0);

	if (swr_init(s->swrCtx) < 0) {
		console->Print("Failed to initialize resampler context\n");
		return false;
	}

	return true;
}

// }}}

// Worker thread {{{

// workerStartRender {{{

static void workerStartRender(AVCodecID videoCodec, AVCodecID audioCodec, int64_t videoBitrate, int64_t audioBitrate, AVDictionary *options) {
	if (avformat_alloc_output_context2(&Renderer::g_render.outCtx, NULL, NULL, Renderer::g_render.filename.c_str()) == AVERROR(EINVAL)) {
		console->Print("Failed to deduce output format from file extension - using AVI\n");
		avformat_alloc_output_context2(&Renderer::g_render.outCtx, NULL, "avi", Renderer::g_render.filename.c_str());
	}

	if (!Renderer::g_render.outCtx) {
		console->Print("Failed to allocate output context\n");
		return;
	}

	if (!addStream(&Renderer::g_render.videoStream, Renderer::g_render.outCtx, videoCodec, videoBitrate, Renderer::g_render.fps, 0, Renderer::g_render.width, Renderer::g_render.height)) {
		console->Print("Failed to create video stream\n");
		avformat_free_context(Renderer::g_render.outCtx);
		return;
	}

	if (!addStream(&Renderer::g_render.audioStream, Renderer::g_render.outCtx, audioCodec, audioBitrate, Renderer::g_render.samplerate, Renderer::g_render.samplerate / 10)) {  // offset the start by 0.1s because idk
		console->Print("Failed to create audio stream\n");
		closeStream(&Renderer::g_render.videoStream);
		avformat_free_context(Renderer::g_render.outCtx);
		return;
	}

	if (!openVideo(Renderer::g_render.outCtx, &Renderer::g_render.videoStream, &options)) {
		console->Print("Failed to open video stream\n");
		closeStream(&Renderer::g_render.audioStream);
		closeStream(&Renderer::g_render.videoStream);
		avformat_free_context(Renderer::g_render.outCtx);
		return;
	}

	if (!openAudio(Renderer::g_render.outCtx, &Renderer::g_render.audioStream, &options)) {
		console->Print("Failed to open audio stream\n");
		closeStream(&Renderer::g_render.audioStream);
		closeStream(&Renderer::g_render.videoStream);
		avformat_free_context(Renderer::g_render.outCtx);
		return;
	}

	if (avio_open(&Renderer::g_render.outCtx->pb, Renderer::g_render.filename.c_str(), AVIO_FLAG_WRITE) < 0) {
		console->Print("Failed to open output file\n");
		closeStream(&Renderer::g_render.audioStream);
		closeStream(&Renderer::g_render.videoStream);
		avformat_free_context(Renderer::g_render.outCtx);
		return;
	}

	if (avformat_write_header(Renderer::g_render.outCtx, &options) < 0) {
		console->Print("Failed to write output file\n");
		closeStream(&Renderer::g_render.audioStream);
		closeStream(&Renderer::g_render.videoStream);
		avio_closep(&Renderer::g_render.outCtx->pb);
		avformat_free_context(Renderer::g_render.outCtx);
		return;
	}

	Renderer::g_render.audioBufIdx = 0;
	Renderer::g_render.audioBufSz = Renderer::g_render.audioStream.tmpFrame->nb_samples;

	Renderer::g_render.channels = Renderer::g_render.audioStream.enc->channels;

	Renderer::g_render.imageBuf = (uint8_t *)malloc(3 * Renderer::g_render.width * Renderer::g_render.height);
	for (int i = 0; i < Renderer::g_render.channels; ++i) {
		Renderer::g_render.audioBuf[i] = (int16_t *)malloc(Renderer::g_render.audioBufSz * sizeof Renderer::g_render.audioBuf[i][0]);
	}

	Renderer::g_render.nextBlendIdx = 0;
	Renderer::g_render.totalBlendWeight = 0;
	if (Renderer::g_render.toBlend > 1) {
		Renderer::g_render.blendSumBuf = (uint32_t *)calloc(3 * Renderer::g_render.width * Renderer::g_render.height, sizeof Renderer::g_render.blendSumBuf[0]);
	}

	g_movieInfo->moviename[0] = 'a';  // Just something nonzero to make the game think there's a movie in progress
	g_movieInfo->moviename[1] = 0;
	g_movieInfo->movieframe = 0;
	g_movieInfo->type = 0;  // Should stop anything actually being output

	Renderer::g_render.isRendering.store(true);

	console->Print("Started rendering to '%s'\n", Renderer::g_render.filename.c_str());

	console->Print(
		"    video: %s (%dx%d @ %d fps, %" PRId64 " kb/s, %s)\n",
		Renderer::g_render.videoStream.codec->name,
		Renderer::g_render.width,
		Renderer::g_render.height,
		Renderer::g_render.fps,
		Renderer::g_render.videoStream.enc->bit_rate / 1000,
		av_get_pix_fmt_name(Renderer::g_render.videoStream.enc->pix_fmt));

	console->Print(
		"    audio: %s (%d Hz, %" PRId64 " kb/s, %s)\n",
		Renderer::g_render.audioStream.codec->name,
		Renderer::g_render.audioStream.enc->sample_rate,
		Renderer::g_render.audioStream.enc->bit_rate / 1000,
		av_get_sample_fmt_name(Renderer::g_render.audioStream.enc->sample_fmt));
}

// }}}

// workerFinishRender {{{

static void workerFinishRender(bool error) {
	if (error) {
		console->Print("A fatal error occurred; stopping render early\n");
	} else {
		console->Print("Stopping render...\n");
	}

	Renderer::g_render.isRendering.store(false);

	flushStream(&Renderer::g_render.videoStream, true);
	flushStream(&Renderer::g_render.audioStream, true);

	av_write_trailer(Renderer::g_render.outCtx);

	console->Print("Rendered %d frames to '%s'\n", Renderer::g_render.videoStream.nextPts, Renderer::g_render.filename.c_str());

	Renderer::g_render.imageBufLock.lock();
	Renderer::g_render.audioBufLock.lock();

	closeStream(&Renderer::g_render.videoStream);
	closeStream(&Renderer::g_render.audioStream);
	avio_closep(&Renderer::g_render.outCtx->pb);
	avformat_free_context(Renderer::g_render.outCtx);
	free(Renderer::g_render.imageBuf);
	for (int i = 0; i < Renderer::g_render.channels; ++i) {
		free(Renderer::g_render.audioBuf[i]);
	}
	if (Renderer::g_render.toBlend > 1) {
		free(Renderer::g_render.blendSumBuf);
	}

	Renderer::g_render.imageBufLock.unlock();
	Renderer::g_render.audioBufLock.unlock();

	// Reset all the Source movieinfo struct to its default values
	g_movieInfo->moviename[0] = 0;
	g_movieInfo->movieframe = 0;
	g_movieInfo->type = MovieInfo_t::FMOVIE_TGA | MovieInfo_t::FMOVIE_WAV;
	g_movieInfo->jpeg_quality = 50;

	host_framerate.SetValue(0.0f);
}

// }}}

// workerHandleVideoFrame {{{

static bool workerHandleVideoFrame() {
	Renderer::g_render.imageBufLock.lock();
	Renderer::g_render.workerMsg.store(Renderer::WorkerMsg::NONE);  // It's important that we do this *after* locking the image buffer
	size_t size = Renderer::g_render.width * Renderer::g_render.height * 3;
	if (Renderer::g_render.toBlend == 1) {
		// We can just copy the data directly
		memcpy(Renderer::g_render.videoStream.tmpFrame->data[0], Renderer::g_render.imageBuf, size);
		Renderer::g_render.imageBufLock.unlock();
	} else {
		if (Renderer::g_render.nextBlendIdx >= Renderer::g_render.toBlendStart && Renderer::g_render.nextBlendIdx < Renderer::g_render.toBlendEnd) {
			double framePos = (Renderer::g_render.nextBlendIdx - Renderer::g_render.toBlendStart) / (Renderer::g_render.toBlendEnd - Renderer::g_render.toBlendStart - 1);
			uint32_t weight = calcFrameWeight(framePos);
			for (size_t i = 0; i < size; ++i) {
				Renderer::g_render.blendSumBuf[i] += weight * (uint32_t)Renderer::g_render.imageBuf[i];
			}
			Renderer::g_render.totalBlendWeight += weight;
		}
		Renderer::g_render.imageBufLock.unlock();

		if (++Renderer::g_render.nextBlendIdx != Renderer::g_render.toBlend) {
			// We've added in this frame, but not done blending yet
			return true;
		}

		for (size_t i = 0; i < size; ++i) {
			Renderer::g_render.videoStream.tmpFrame->data[0][i] = (uint8_t)(Renderer::g_render.blendSumBuf[i] / Renderer::g_render.totalBlendWeight);
			Renderer::g_render.blendSumBuf[i] = 0;
		}

		Renderer::g_render.nextBlendIdx = 0;
		Renderer::g_render.totalBlendWeight = 0;
	}

	// tmpFrame is now our final frame; convert to the output format and
	// process it

	sws_scale(Renderer::g_render.videoStream.swsCtx, (const uint8_t *const *)Renderer::g_render.videoStream.tmpFrame->data, Renderer::g_render.videoStream.tmpFrame->linesize, 0, Renderer::g_render.height, Renderer::g_render.videoStream.frame->data, Renderer::g_render.videoStream.frame->linesize);

	Renderer::g_render.videoStream.frame->pts = Renderer::g_render.videoStream.nextPts;

	if (avcodec_send_frame(Renderer::g_render.videoStream.enc, Renderer::g_render.videoStream.frame) < 0) {
		console->Print("Failed to send video frame for encoding!\n");
		return false;
	}

	if (!flushStream(&Renderer::g_render.videoStream)) {
		console->Print("Failed to flush video stream!\n");
		return false;
	}

	++Renderer::g_render.videoStream.nextPts;

	return true;
}

// }}}

// workerHandleAudioFrame {{{

static bool workerHandleAudioFrame() {
	Renderer::g_render.audioBufLock.lock();
	Renderer::g_render.workerMsg.store(Renderer::WorkerMsg::NONE);  // It's important that we do this *after* locking the audio buffer

	Renderer::Stream *s = &Renderer::g_render.audioStream;
	for (int i = 0; i < Renderer::g_render.channels; ++i) {
		memcpy(s->tmpFrame->data[i], Renderer::g_render.audioBuf[i], Renderer::g_render.audioBufSz * sizeof Renderer::g_render.audioBuf[i][0]);
	}
	Renderer::g_render.audioBufLock.unlock();


	s->tmpFrame->pts = s->nextPts;
	s->nextPts += s->frame->nb_samples;

	if (av_frame_make_writable(s->frame) < 0) {
		console->Print("Failed to make audio frame writable!\n");
		return false;
	}

	if (swr_convert(s->swrCtx, s->frame->data, s->frame->nb_samples, (const uint8_t **)s->tmpFrame->data, s->tmpFrame->nb_samples) < 0) {
		console->Print("Failed to resample audio frame!\n");
		return false;
	}

	s->frame->pts = av_rescale_q(s->tmpFrame->pts, {1, s->enc->sample_rate}, s->enc->time_base);

	if (avcodec_send_frame(s->enc, s->frame) < 0) {
		console->Print("Failed to send audio frame for encoding!\n");
		return false;
	}

	if (!flushStream(s)) {
		console->Print("Failed to flush audio stream!\n");
		return false;
	}

	return true;
}

// }}}

static void worker(AVCodecID videoCodec, AVCodecID audioCodec, int64_t videoBitrate, int64_t audioBitrate, AVDictionary *options) {
	workerStartRender(videoCodec, audioCodec, videoBitrate, audioBitrate, options);
	if (!Renderer::g_render.isRendering.load()) {
		Renderer::g_render.workerFailedToStart.store(true);
		return;
	}
	std::unique_lock<std::mutex> lock(Renderer::g_render.workerUpdateLock);
	while (true) {
		Renderer::g_render.workerUpdate.wait(lock);
		switch (Renderer::g_render.workerMsg.load()) {
		case Renderer::WorkerMsg::VIDEO_FRAME_READY:
			if (!workerHandleVideoFrame()) {
				workerFinishRender(true);
				return;
			}
			break;
		case Renderer::WorkerMsg::AUDIO_FRAME_READY:
			if (!workerHandleAudioFrame()) {
				workerFinishRender(true);
				return;
			}
			break;
		case Renderer::WorkerMsg::STOP_RENDERING_ERROR:
			workerFinishRender(true);
			return;
		case Renderer::WorkerMsg::STOP_RENDERING_REQUESTED:
			workerFinishRender(false);
			return;
		case Renderer::WorkerMsg::NONE:
			break;
		}
	}
}

// }}}

// startRender {{{

static void startRender() {
	// We can't start rendering if we haven't stopped yet, so make sure
	// the worker thread isn't running
	if (Renderer::g_render.worker.joinable()) {
		Renderer::g_render.worker.join();
	}

	AVDictionary *options = NULL;

	if (Renderer::g_render.isRendering.load()) {
		console->Print("Already rendering\n");
		return;
	}

	if (!sv_cheats.GetBool()) {
		console->Print("sv_cheats must be enabled\n");
		return;
	}

	Renderer::g_render.samplerate = p2fx_render_sample_rate.GetInt();
	Renderer::g_render.fps = p2fx_render_fps.GetInt();

	if (p2fx_render_blend.GetInt() == 0 && host_framerate.GetInt() == 0) {
		console->Print("host_framerate or p2fx_render_blend must be nonzero\n");
		return;
	} else if (p2fx_render_blend.GetInt() != 0) {
		Renderer::g_render.toBlend = p2fx_render_blend.GetInt();
		int framerate = Renderer::g_render.toBlend * Renderer::g_render.fps;
		if (host_framerate.GetInt() != 0 && host_framerate.GetInt() != framerate) {
			console->Print("Warning: overriding host_framerate to %d based on p2fx_render_fps and p2fx_render_blend\n", framerate);
		}
		host_framerate.SetValue(framerate);
	} else {  // host_framerate nonzero
		int framerate = host_framerate.GetInt();
		if (framerate % Renderer::g_render.fps != 0) {
			console->Print("host_framerate must be a multiple of p2fx_render_fps\n");
			return;
		}
		Renderer::g_render.toBlend = framerate / Renderer::g_render.fps;
	}

	{
		float shutter = (float)p2fx_render_shutter_angle.GetInt() / 360.0f;
		int toExclude = (int)round(Renderer::g_render.toBlend * (1 - shutter) / 2);
		if (toExclude * 2 >= Renderer::g_render.toBlend) {
			toExclude = Renderer::g_render.toBlend / 2 - 1;
		}
		Renderer::g_render.toBlendStart = toExclude;
		Renderer::g_render.toBlendEnd = Renderer::g_render.toBlend - toExclude;
	}

	if (snd_surround_speakers.GetInt() != 2) {
		console->Print("Note: setting speaker configuration to stereo. You may wish to reset it after the render\n");
		snd_surround_speakers.SetValue(2);
	}

	AVCodecID videoCodec = videoCodecFromName(p2fx_render_vcodec.GetString());
	AVCodecID audioCodec = audioCodecFromName(p2fx_render_acodec.GetString());

	if (videoCodec == AV_CODEC_ID_NONE) {
		console->Print("Unknown video codec '%s'\n", p2fx_render_vcodec.GetString());
		return;
	}

	if (audioCodec == AV_CODEC_ID_NONE) {
		console->Print("Unknown audio codec '%s'\n", p2fx_render_acodec.GetString());
		return;
	}

	float videoBitrate = p2fx_render_vbitrate.GetFloat();
	float audioBitrate = p2fx_render_abitrate.GetFloat();

	int quality = p2fx_render_quality.GetInt();
	if (quality < 0) quality = 0;
	if (quality > 50) quality = 50;

	// Quality options
	{
		// TODO: CRF scales non-linearly. Do we make this conversion
		// non-linear to better reflect that?

		int min = 0;
		int max = 63;
		if (videoCodec == AV_CODEC_ID_H264 || videoCodec == AV_CODEC_ID_HEVC) {
			max = 51;
		} else if (videoCodec == AV_CODEC_ID_VP8) {
			min = 4;
		}
		int crf = min + (max - min) * (50 - quality) / 50;
		std::string crfStr = std::to_string(crf);
		av_dict_set(&options, "crf", crfStr.c_str(), 0);

		if (videoCodec == AV_CODEC_ID_H264 || videoCodec == AV_CODEC_ID_HEVC) {
			av_dict_set(&options, "preset", "slower", 0);
		}
	}

	Renderer::g_render.width = GetScreenWidth();
	Renderer::g_render.height = GetScreenHeight();

	Renderer::g_render.workerFailedToStart.store(false);

	Renderer::g_render.workerMsg.store(Renderer::WorkerMsg::NONE);
	Renderer::g_render.worker = std::thread(worker, videoCodec, audioCodec, videoBitrate * 1000, audioBitrate * 1000, options);

	// Busy-wait until the rendering has started so that we don't miss
	// any frames
	while (!Renderer::g_render.isRendering.load() && !Renderer::g_render.workerFailedToStart.load())
		;
}

// }}}

// Audio output {{{

static inline short clip16(int x) {
	if (x < -32768) return -32768;  // Source uses -32767, but that's, like, not how two's complement works
	if (x > 32767) return 32767;
	return x;
}

static void (*SND_RecordBuffer)();
static void SND_RecordBuffer_Hook();
static Hook g_RecordBufferHook(&SND_RecordBuffer_Hook);
static void SND_RecordBuffer_Hook() {
	if (!Renderer::g_render.isRendering.load()) {
		g_RecordBufferHook.Disable();
		SND_RecordBuffer();
		g_RecordBufferHook.Enable();
		return;
	}

	if (Renderer::g_render.isPaused.load())
		return;

	if (engine->ConsoleVisible()) return;

	// If the worker has received a message it hasn't yet handled,
	// busy-wait; this is a really obscure race condition that should
	// never happen
	while (Renderer::g_render.isRendering.load() && Renderer::g_render.workerMsg.load() != Renderer::WorkerMsg::NONE)
		;

	if (snd_surround_speakers.GetInt() != 2) {
		console->Print("Speaker configuration changed!\n");
		msgStopRender(true);
		return;
	}

	Renderer::g_render.audioBufLock.lock();

	for (int i = 0; i < *g_snd_linear_count; i += Renderer::g_render.channels) {
		for (int c = 0; c < Renderer::g_render.channels; ++c) {
			Renderer::g_render.audioBuf[c][Renderer::g_render.audioBufIdx] = clip16(((*g_snd_p)[i + c] * *g_snd_vol) >> 8);
		}

		++Renderer::g_render.audioBufIdx;
		if (Renderer::g_render.audioBufIdx == Renderer::g_render.audioBufSz) {
			Renderer::g_render.audioBufLock.unlock();

			Renderer::g_render.workerUpdateLock.lock();
			Renderer::g_render.workerMsg.store(Renderer::WorkerMsg::AUDIO_FRAME_READY);
			Renderer::g_render.workerUpdate.notify_all();
			Renderer::g_render.workerUpdateLock.unlock();

			// Busy-wait until the worker locks the audio buffer; not
			// ideal, but shouldn't take long
			while (Renderer::g_render.isRendering.load() && Renderer::g_render.workerMsg.load() != Renderer::WorkerMsg::NONE)
				;

			if (!Renderer::g_render.isRendering.load()) {
				// We shouldn't write to that buffer anymore, it's been
				// invalidated
				// We've already unlocked audioBufLock so just return
				return;
			}

			Renderer::g_render.audioBufLock.lock();
			Renderer::g_render.audioBufIdx = 0;
		}
	}

	Renderer::g_render.audioBufLock.unlock();

	return;
}

// }}}

// Video output {{{

void Renderer::Frame() {
	if (Renderer::isDemoLoading && p2fx_render_autostart.GetBool()) {
		bool start = engine->demoplayer->IsPlaybackFixReady();
		start &= session->isRunning;
		if (engine->GetMaxClients() >= 2 && p2fx_render_skip_coop_videos.GetBool()) {
			start &= engine->startedTransitionFadeout;
		}
		Renderer::g_render.isPaused.store(!start);
		if (!start) return;

		Renderer::isDemoLoading = false;

		if (!Renderer::g_render.isRendering.load()) {
			Renderer::g_render.filename = std::string(engine->GetGameDirectory()) + "/" + std::string(engine->demoplayer->DemoName) + "." + p2fx_render_autostart_extension.GetString();
			startRender();
		}
	} else {
		Renderer::isDemoLoading = false;
	}

	if (!Renderer::g_render.isRendering.load()) return;

	if (engine->GetMaxClients() >= 2 && p2fx_render_skip_coop_videos.GetBool() && engine->isLevelTransition) {
		// The transition video has begun, pause the render
		Renderer::g_render.isPaused.store(true);
		return;
	}

	// autostop: if it's the __END__ tick, or the demo is over, stop
	// rendering


	if (p2fx_render_autostop.GetBool() && Renderer::segmentEndTick != -1 && engine->demoplayer->IsPlaying() && engine->demoplayer->GetTick() > Renderer::segmentEndTick) {
		if (!p2fx_render_merge.GetBool())
			msgStopRender(false);
		else
			Renderer::g_render.isPaused.store(true);
		return;
	}

	if (Renderer::g_render.isPaused.load())
		return;

	// Don't render if the console is visible
	if (engine->ConsoleVisible()) return;

	// If the worker has received a message it hasn't yet handled,
	// busy-wait; this is a really obscure race condition that should
	// never happen
	while (Renderer::g_render.isRendering.load() && Renderer::g_render.workerMsg.load() != Renderer::WorkerMsg::NONE)
		;

	if (GetScreenWidth() != Renderer::g_render.width) {
		console->Print("Screen resolution has changed!\n");
		msgStopRender(true);
		return;
	}

	if (GetScreenHeight() != Renderer::g_render.height) {
		console->Print("Screen resolution has changed!\n");
		msgStopRender(true);
		return;
	}

	if (av_frame_make_writable(Renderer::g_render.videoStream.frame) < 0) {
		console->Print("Failed to make video frame writable!\n");
		msgStopRender(true);
		return;
	}

	Renderer::g_render.imageBufLock.lock();

	// Double check the buffer hasn't at some point between the start of
	// the function and now been invalidated
	if (!Renderer::g_render.isRendering.load()) {
		Renderer::g_render.imageBufLock.unlock();
		return;
	}

	ReadScreenPixels(0, 0, Renderer::g_render.width, Renderer::g_render.height, Renderer::g_render.imageBuf, IMAGE_FORMAT_BGR888);

	Renderer::g_render.imageBufLock.unlock();

	// Signal to the worker thread that there is image data for it to
	// process
	{
		std::lock_guard<std::mutex> lock(Renderer::g_render.workerUpdateLock);
		Renderer::g_render.workerMsg.store(Renderer::WorkerMsg::VIDEO_FRAME_READY);
		Renderer::g_render.workerUpdate.notify_all();
	}
}

// }}}

ON_EVENT(DEMO_STOP) {
	if (Renderer::g_render.isRendering.load() && p2fx_render_autostop.GetBool() && !p2fx_render_merge.GetBool()) {
		msgStopRender(false);
	}
}

// Init {{{

void Renderer::Init(void **videomode) {
	g_videomode = videomode;

	snd_surround_speakers = Variable("snd_surround_speakers");

#ifdef _WIN32
	SND_RecordBuffer = (void (*)())Memory::Scan(engine->Name(), "55 8B EC 80 3D ? ? ? ? 00 53 56 57 0F 84 15 01 00 00 E8 ? ? ? ? 84 C0 0F 85 08 01 00 00 A1 ? ? ? ? 3B 05");
	g_movieInfo = *(MovieInfo_t **)((uintptr_t)SND_RecordBuffer + 5);
#else
	if (p2fx.game->Is(SourceGame_Portal2)) {
		SND_RecordBuffer = (void (*)())Memory::Scan(engine->Name(), "80 3D ? ? ? ? 00 75 07 C3 ? ? ? ? ? ? 55 89 E5 57 56 53 83 EC 1C E8 ? ? ? ? 84 C0 0F 85 ? ? ? ?");
	} else if (p2fx.game->Is(SourceGame_PortalReloaded) || p2fx.game->Is(SourceGame_PortalStoriesMel)) {
		SND_RecordBuffer = (void (*)())Memory::Scan(engine->Name(), "55 89 E5 57 56 53 83 EC 3C 65 A1 ? ? ? ? 89 45 E4 31 C0 E8 ? ? ? ? 84 C0 75 1B");
	} else {  // Pre-update engine
		SND_RecordBuffer = (void (*)())Memory::Scan(engine->Name(), "55 89 E5 57 56 53 83 EC 2C E8 ? ? ? ? 84 C0 75 0E 8D 65 F4 5B 5E 5F 5D C3");
	}

	if (p2fx.game->Is(SourceGame_Portal2)) {
		g_movieInfo = *(MovieInfo_t **)((uintptr_t)SND_RecordBuffer + 2);
	} else if (p2fx.game->Is(SourceGame_PortalReloaded) || p2fx.game->Is(SourceGame_PortalStoriesMel)) {
		uintptr_t SND_IsRecording = Memory::Read((uintptr_t)SND_RecordBuffer + 21);
		g_movieInfo = *(MovieInfo_t **)(SND_IsRecording + 2);
	} else {  // Pre-update engine
		uintptr_t SND_IsRecording = Memory::Read((uintptr_t)SND_RecordBuffer + 10);
		g_movieInfo = *(MovieInfo_t **)(SND_IsRecording + 11);
	}
#endif

	g_RecordBufferHook.SetFunc(SND_RecordBuffer);

#ifndef _WIN32
	if (p2fx.game->Is(SourceGame_Portal2)) {
		uint32_t fn = (uint32_t)SND_RecordBuffer;
		uint32_t base = fn + 11 + *(uint32_t *)(fn + 13);
		g_snd_linear_count = (int *)(base + *(uint32_t *)(fn + 49));
		g_snd_p = (int **)(base + *(uint32_t *)(fn + 96));
		g_snd_vol = (int *)(base + *(uint32_t *)(fn + 102));
	} else
#endif
	{
		g_snd_linear_count = *(int **)((uintptr_t)SND_RecordBuffer + Offsets::snd_linear_count);
		g_snd_p = *(int ***)((uintptr_t)SND_RecordBuffer + Offsets::snd_p);
		g_snd_vol = *(int **)((uintptr_t)SND_RecordBuffer + Offsets::snd_vol);
	}

	Command::Hook("startmovie", &startmovie_cbk, startmovie_origCbk);
	Command::Hook("endmovie", &endmovie_cbk, endmovie_origCbk);
}

void Renderer::Cleanup() {
	g_RecordBufferHook.Disable();
	Command::Unhook("startmovie", startmovie_origCbk);
	Command::Unhook("endmovie", endmovie_origCbk);
}

// }}}

// Commands {{{

CON_COMMAND(p2fx_render_start, "p2fx_render_start <file> - start rendering frames to the given video file\n") {
	if (args.ArgC() != 2) {
		console->Print(p2fx_render_start.ThisPtr()->m_pszHelpString);
		return;
	}

	Renderer::g_render.filename = std::string(args[1]);

	startRender();
}

CON_COMMAND(p2fx_render_finish, "p2fx_render_finish - stop rendering frames\n") {
	if (args.ArgC() != 1) {
		console->Print(p2fx_render_finish.ThisPtr()->m_pszHelpString);
		return;
	}

	if (!Renderer::g_render.isRendering.load()) {
		console->Print("Not rendering!\n");
		return;
	}

	msgStopRender(false);
}

// }}}
