#include "network.h"

static Trace s_trace;
Trace& _trace = s_trace;

Trace::Trace():
	m_severity(TRACE_DEBUG)
{
}	

void Trace::f(Severity severity, const wchar_t* fmt, ...)
{
	if (severity < m_severity)
		return;

	va_list args;
	va_start(args, fmt);
	write(severity, fmt, args);
	va_end(args);
}

void Trace::d(const wchar_t* fmt, ...)
{
	if (TRACE_DEBUG < m_severity)
		return;

	va_list args;
	va_start(args, fmt);
	write(TRACE_DEBUG, fmt, args);
	va_end(args);
}

void Trace::i(const wchar_t* fmt, ...)
{
	if (TRACE_INFO < m_severity)
		return;

	va_list args;
	va_start(args, fmt);
	write(TRACE_INFO, fmt, args);
	va_end(args);
}

void Trace::w(const wchar_t* fmt, ...)
{
	if (TRACE_WARNING < m_severity)
		return;

	va_list args;
	va_start(args, fmt);
	write(TRACE_WARNING, fmt, args);
	va_end(args);
}

void Trace::e(const wchar_t* fmt, ...)
{
	if (TRACE_ERROR < m_severity)
		return;

	va_list args;
	va_start(args, fmt);
	write(TRACE_ERROR, fmt, args);
	va_end(args);
}

void Trace::write(Severity severity, const wchar_t* fmt, va_list args)
{
	vfwprintf(stdout, fmt, args);
}
