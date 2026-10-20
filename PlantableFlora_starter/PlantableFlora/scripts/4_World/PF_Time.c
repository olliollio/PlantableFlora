class PF_Time
{
	// Seconds since 2020-01-01 00:00 UTC. Vanilla has no epoch clock, only the UTC date and time,
	// so build one. Wall-clock (not server uptime) so growth continues while the server is down.
	static int Now()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		return DaysSince2020(year, month, day) * 86400 + hour * 3600 + minute * 60 + second;
	}

	// civil date -> day number (Howard Hinnant's days_from_civil), shifted so 2020-01-01 = 0
	static int DaysSince2020(int year, int month, int day)
	{
		if (month <= 2)
			year = year - 1;
		int era = year / 400;
		int yearOfEra = year - era * 400;
		int monthPart = (month + 9) % 12;
		int dayOfYear = (153 * monthPart + 2) / 5 + day - 1;
		int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
		return era * 146097 + dayOfEra - 719468 - 18262;
	}
}
