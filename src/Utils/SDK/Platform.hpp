#pragma once

template <class T>
inline T *Construct(T *pMemory) {
	return ::new (pMemory) T;
}

template <class T, typename ARG1>
inline T *Construct(T *pMemory, ARG1 a1) {
	return ::new (pMemory) T(a1);
}

template <class T, typename ARG1, typename ARG2>
inline T *Construct(T *pMemory, ARG1 a1, ARG2 a2) {
	return ::new (pMemory) T(a1, a2);
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T *Construct(T *pMemory, ARG1 a1, ARG2 a2, ARG3 a3) {
	return ::new (pMemory) T(a1, a2, a3);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T *Construct(T *pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4) {
	return ::new (pMemory) T(a1, a2, a3, a4);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T *Construct(T *pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5) {
	return ::new (pMemory) T(a1, a2, a3, a4, a5);
}

template <class T>
inline T *CopyConstruct(T *pMemory, T const &src) {
	return ::new (pMemory) T(src);
}

template <class T>
inline void Destruct(T *pMemory) {
	pMemory->~T();
}