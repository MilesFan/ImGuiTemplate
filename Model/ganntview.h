#ifndef GANNTVIEW_H
#define GANNTVIEW_H
#include <time.h>
time_t make_time(int year, int month, int day,
	int hour = 0, int min = 0, int sec = 0) {
	tm t{};
	t.tm_year = year - 1900;   // 年要从 1900 算起
	t.tm_mon = month - 1;     // 月 0-11
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = min;
	t.tm_sec = sec;
	t.tm_isdst = -1;           // 让系统自动判断夏令时
	return mktime(&t);         // 本地时区
}
namespace Task
{
	class GanntView {
	public:
		time_t StartDate;
		time_t EndDate;

		GanntView(time_t StartDate, time_t EndDate) :
				  StartDate(StartDate), EndDate(EndDate) {}
	};
}
#endif // !GANNTVIEW_H