/*****************************************************************************
* Copyright (c) 2014 Ted John
* OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
*
* This file is part of OpenRCT2.
*
* OpenRCT2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef _STRINGS_H_
#define _STRINGS_H_

typedef unsigned short rct_string_id;

void format_string(char* dest, rct_string_id format, void* args);
void generate_string_file();

enum {
	STR_NONE = -1,

	STR_DATE_DAY_1 = 779,
	STR_DATE_DAY_2 = STR_DATE_DAY_1 + 1,
	STR_DATE_DAY_3 = STR_DATE_DAY_1 + 2,
	STR_DATE_DAY_4 = STR_DATE_DAY_1 + 3,
	STR_DATE_DAY_5 = STR_DATE_DAY_1 + 4,
	STR_DATE_DAY_6 = STR_DATE_DAY_1 + 5,
	STR_DATE_DAY_7 = STR_DATE_DAY_1 + 6,
	STR_DATE_DAY_8 = STR_DATE_DAY_1 + 7,
	STR_DATE_DAY_9 = STR_DATE_DAY_1 + 8,
	STR_DATE_DAY_10 = STR_DATE_DAY_1 + 9,
	STR_DATE_DAY_11 = STR_DATE_DAY_1 + 10,
	STR_DATE_DAY_12 = STR_DATE_DAY_1 + 11,
	STR_DATE_DAY_13 = STR_DATE_DAY_1 + 12,
	STR_DATE_DAY_14 = STR_DATE_DAY_1 + 13,
	STR_DATE_DAY_15 = STR_DATE_DAY_1 + 14,
	STR_DATE_DAY_16 = STR_DATE_DAY_1 + 15,
	STR_DATE_DAY_17 = STR_DATE_DAY_1 + 16,
	STR_DATE_DAY_18 = STR_DATE_DAY_1 + 17,
	STR_DATE_DAY_19 = STR_DATE_DAY_1 + 18,
	STR_DATE_DAY_20 = STR_DATE_DAY_1 + 19,
	STR_DATE_DAY_21 = STR_DATE_DAY_1 + 20,
	STR_DATE_DAY_22 = STR_DATE_DAY_1 + 21,
	STR_DATE_DAY_23 = STR_DATE_DAY_1 + 22,
	STR_DATE_DAY_24 = STR_DATE_DAY_1 + 23,
	STR_DATE_DAY_25 = STR_DATE_DAY_1 + 24,
	STR_DATE_DAY_26 = STR_DATE_DAY_1 + 25,
	STR_DATE_DAY_27 = STR_DATE_DAY_1 + 26,
	STR_DATE_DAY_28 = STR_DATE_DAY_1 + 27,
	STR_DATE_DAY_29 = STR_DATE_DAY_1 + 28,
	STR_DATE_DAY_30 = STR_DATE_DAY_1 + 29,
	STR_DATE_DAY_31 = STR_DATE_DAY_1 + 30,
	STR_MONTH_SHORT_JAN = 810,
	STR_MONTH_SHORT_FEB = STR_MONTH_SHORT_JAN + 1,
	STR_MONTH_SHORT_MAR = STR_MONTH_SHORT_JAN + 2,
	STR_MONTH_SHORT_APR = STR_MONTH_SHORT_JAN + 3,
	STR_MONTH_SHORT_MAY = STR_MONTH_SHORT_JAN + 4,
	STR_MONTH_SHORT_JUN = STR_MONTH_SHORT_JAN + 5,
	STR_MONTH_SHORT_JUL = STR_MONTH_SHORT_JAN + 6,
	STR_MONTH_SHORT_AUG = STR_MONTH_SHORT_JAN + 7,
	STR_MONTH_SHORT_SEP = STR_MONTH_SHORT_JAN + 8,
	STR_MONTH_SHORT_OCT = STR_MONTH_SHORT_JAN + 9,
	STR_MONTH_SHORT_NOV = STR_MONTH_SHORT_JAN + 10,
	STR_MONTH_SHORT_DEC = STR_MONTH_SHORT_JAN + 11,

	STR_CLOSE_WINDOW_TIP = 828,
	STR_WINDOW_TITLE_TIP = 829,

	STR_ZOOM_IN_TIP = 830,
	STR_ZOOM_OUT_TIP = 831,
	STR_ROTATE_TIP = 832,
	STR_PAUSE_GAME_TIP = 833,
	STR_DISC_AND_GAME_OPTIONS_TIP = 834,

	STR_VIEW_OPTIONS_TIP = 937,
	STR_ADJUST_LAND_TIP = 938,

	STR_CANCEL = 972,
	STR_OK = 973,

	STR_RAISE_COST_AMOUNT = 984,
	STR_LOWER_COST_AMOUNT = 985,
	STR_COST_AMOUNT = 986,

	STR_LOCATE_SUBJECT_TIP = 1027,

	STR_RIDES_IN_PARK_TIP = 1053,

	STR_PLACE_SCENERY_TIP = 1159,
	STR_ADJUST_WATER_TIP = 1160,

	STR_BUILD_FOOTPATH_TIP = 1173,

	STR_GUESTS = 1463,

	STR_GUESTS_TIP = 1693,
	STR_STAFF_TIP = 1694,

	STR_INDIVIDUAL_GUESTS_TIP = 1752,
	STR_SUMMARISED_GUESTS_TIP = 1753,

	STR_ACTIONS = 1814,
	STR_THOUGHTS = STR_ACTIONS + 1,
	STR_INFORMATION_TYPE_TIP = 1816,
	STR_GUESTS_COUNT_COMMA_SEP = 1817,
	STR_ALL_GUESTS = 1818,
	STR_ALL_GUESTS_SUMMARISED = 1819,
	STR_GUESTS_FILTER = 1820,
	STR_GUESTS_FILTER_THINKING = STR_GUESTS_FILTER + 1,
	STR_GUESTS_FILTER_THINKING_ABOUT = STR_GUESTS_FILTER + 2,

	STR_NUM_GUESTS = 1846,

	STR_BUILD_RIDE_TIP = 1895,

	STR_START_NEW_GAME_TIP = 1921,
	STR_CONTINUE_SAVED_GAME_TIP = 1922,
	STR_SHOW_TUTORIAL_TIP = 1923,
	STR_EXIT = 1924,

	STR_SHOW_SUBJECT_TIP = 1937,

	STR_CELCIUS_VALUE = 2216,
	STR_FAHRENHEIT_VALUE = 2217,

	STR_PARK_rating_TIP = 2221,

	STR_PARK_INFORMATION_TIP = 2233,

	STR_MONTH_JANUARY = 2236,
	STR_MONTH_FEBRUARY = STR_MONTH_JANUARY + 1,
	STR_MONTH_MARCH = STR_MONTH_JANUARY + 2,
	STR_MONTH_APRIL = STR_MONTH_JANUARY + 3,
	STR_MONTH_MAY = STR_MONTH_JANUARY + 4,
	STR_MONTH_JUNE = STR_MONTH_JANUARY + 5,
	STR_MONTH_JULY = STR_MONTH_JANUARY + 6,
	STR_MONTH_AUGUST = STR_MONTH_JANUARY + 7,
	STR_MONTH_SEPTEMBER = STR_MONTH_JANUARY + 8,
	STR_MONTH_OCTOBER = STR_MONTH_JANUARY + 9,
	STR_MONTH_NOVEMBER = STR_MONTH_JANUARY + 10,
	STR_MONTH_DECEMBER = STR_MONTH_JANUARY + 11,

	STR_SELECT_SCENARIO = 2291,

	STR_CHANGE_BASE_LAND_TIP = 2294,
	STR_CHANGE_VERTICAL_LAND_TIP = 2295,

	STR_ADJUST_SMALLER_LAND_TIP = 2378,
	STR_ADJUST_LARGER_LAND_TIP = 2379,
	STR_ADJUST_SMALLER_WATER_TIP = 2380,
	STR_ADJUST_LARGER_WATER_TIP = 2381,
	STR_LAND = 2382,
	STR_WATER = 2383,

	STR_OBJECTIVE_NONE = 2385,
	STR_OBJECTIVE_GUESTS_BY = STR_OBJECTIVE_NONE + 1,
	STR_OBJECTIVE_PARK_VALUE_BY = STR_OBJECTIVE_NONE + 2,
	STR_OBJECTIVE_HAVE_FUN = STR_OBJECTIVE_NONE + 3,
	STR_OBJECTIVE_BUILD_THE_BEST = STR_OBJECTIVE_NONE + 4,
	STR_OBJECTIVE_10_ROLLERCOASTERS = STR_OBJECTIVE_NONE + 5,
	STR_OBJECTIVE_GUESTS_AND_rating = STR_OBJECTIVE_NONE + 6,
	STR_OBJECTIVE_MONTHLY_RIDE_INCOME = STR_OBJECTIVE_NONE + 7,
	STR_OBJECTIVE_10_ROLLERCOASTERS_LENGTH = STR_OBJECTIVE_NONE + 8,
	STR_OBJECTIVE_FINISH_5_ROLLERCOASTERS = STR_OBJECTIVE_NONE + 9,
	STR_OBJECTIVE_REPLAY_LOAN_AND_PARK_VALUE = STR_OBJECTIVE_NONE + 10,
	STR_OBJECTIVE_MONTHLY_FOOD_INCOME = STR_OBJECTIVE_NONE + 11,

	STR_PROFIT_PER_WEEK_AND_PARK_VALUE_TIP = 2482,

	STR_COMPLETED_BY_WITH_COMPANY_VALUE = 2794,

	STR_SHOW_GUESTS_ON_MAP_TIP = 2803,
	STR_SHOW_MAP_TIP = 2805,

	STR_TUTORIAL = 2856,
	STR_PRESS_KEY_OR_MOUSE_BUTTON_FOR_CONTROL = 2857,

	STR_LICENCE_AGREEMENT_NOTICE_1 = 2969,
	STR_LICENCE_AGREEMENT_NOTICE_2 = 2970,

	STR_BEGINNER_PARKS = 3064,
	STR_CHALLENGING_PARKS = STR_BEGINNER_PARKS + 1,
	STR_EXPERT_PARKS = STR_BEGINNER_PARKS + 2,
	STR_REAL_PARKS = STR_BEGINNER_PARKS + 3,
	STR_OTHER_PARKS = STR_BEGINNER_PARKS + 4,

	STR_SCROLL_LEFT_TIP = 3145,
	STR_SCROLL_RIGHT_TIP = STR_SCROLL_LEFT_TIP + 1,
	STR_SCROLL_LEFT_FAST_TIP = STR_SCROLL_LEFT_TIP + 2,
	STR_SCROLL_RIGHT_FAST_TIP = STR_SCROLL_LEFT_TIP + 3,
	STR_SCROLL_LEFT_RIGHT_TIP = STR_SCROLL_LEFT_TIP + 4,
	STR_SCROLL_UP_TIP = STR_SCROLL_LEFT_TIP + 5,
	STR_SCROLL_DOWN_TIP = STR_SCROLL_LEFT_TIP + 6,
	STR_SCROLL_UP_FAST_TIP = STR_SCROLL_LEFT_TIP + 7,
	STR_SCROLL_DOWN_FAST_TIP = STR_SCROLL_LEFT_TIP + 8,
	STR_SCROLL_UP_DOWN_TIP = STR_SCROLL_LEFT_TIP + 9,


	STR_LIST = 3159,

	STR_OBJECTIVE = 3322,

	STR_GAME_TOOLS = 3341,
	STR_SCENARIO_EDITOR = 3342,
	STR_CONVERT_SAVED_GAME_TO_SCENARIO = 3343,
	STR_ROLLER_COASTER_DESIGNER = 3344,
	STR_TRACK_DESIGNS_MANAGER = 3345,

	STR_TUTORIAL_BEGINNERS = 3385,
	STR_TUTORIAL_CUSTOM_RIDES = 3386,
	STR_TUTORIAL_ROLLER_COASTER = 3387,

	STR_CLEAR_SCENERY_TIP = 3437,

	STR_PAGE_1 = 3440,
	STR_PAGE_2 = STR_PAGE_1 + 1,
	STR_PAGE_3 = STR_PAGE_1 + 2,
	STR_PAGE_4 = STR_PAGE_1 + 3,
	STR_PAGE_5 = STR_PAGE_1 + 4,
};

#endif