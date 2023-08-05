#pragma once

#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
};

namespace Renderer {
	struct Stream {
		AVStream *stream;
		AVCodecContext *enc;
		AVCodec *codec;
		AVFrame *frame, *tmpFrame;
		SwsContext *swsCtx;
		SwrContext *swrCtx;
		int nextPts;
	};

	enum class WorkerMsg {
		NONE,
		VIDEO_FRAME_READY,
		AUDIO_FRAME_READY,
		STOP_RENDERING_ERROR,
		STOP_RENDERING_REQUESTED,
	};

	// The global renderer state
	inline struct {
		std::atomic<bool> isRendering;
		std::atomic<bool> isPaused;
		int fps;
		int samplerate;
		int channels;
		Stream videoStream;
		Stream audioStream;
		std::string filename;
		AVFormatContext *outCtx;
		int width, height;

		// The audio stream's temporary frame needs nb_samples worth of
		// audio before we resample and submit, so we keep track of how far
		// in we are with this
		int16_t *audioBuf[8];  // Temporary buffer - contains the planar audio info from the game
		size_t audioBufSz;
		size_t audioBufIdx;

		// This buffer stores the image data
		uint8_t *imageBuf;  // Temporary buffer - contains the raw pixel data read from the screen

		int toBlend;
		int toBlendStart;       // Inclusive
		int toBlendEnd;         // Exclusive
		int nextBlendIdx;       // How many frames in this blend have we seen so far?
		uint32_t *blendSumBuf;  // Blending buffer - contains the sum of the pixel values during blending (we only divide at the end of the blend to prevent rounding errors). Not allocated if toBlend == 1.
		uint32_t totalBlendWeight;

		// Synchronisation
		std::thread worker;
		std::mutex workerUpdateLock;
		std::condition_variable workerUpdate;
		std::atomic<WorkerMsg> workerMsg;
		std::mutex imageBufLock;
		std::mutex audioBufLock;
		std::atomic<bool> workerFailedToStart;
	} g_render;

	void Frame();
	void Init(void **videomode);
	void Cleanup();
	extern int segmentEndTick;
	extern bool isDemoLoading;
};  // namespace Renderer
