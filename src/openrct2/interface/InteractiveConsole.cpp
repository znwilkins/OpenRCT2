/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "InteractiveConsole.h"

#include "../Context.h"
#include "../EditorObjectSelectionSession.h"
#include "../Game.h"
#include "../OpenRCT2.h"
#include "../PlatformEnvironment.h"
#include "../ReplayManager.h"
#include "../Version.h"
#include "../actions/ClimateSetAction.hpp"
#include "../actions/RideSetPriceAction.hpp"
#include "../actions/RideSetSetting.hpp"
#include "../actions/SetCheatAction.hpp"
#include "../actions/StaffSetCostumeAction.hpp"
#include "../config/Config.h"
#include "../core/Guard.hpp"
#include "../core/Path.hpp"
#include "../core/String.hpp"
#include "../drawing/Drawing.h"
#include "../drawing/Font.h"
#include "../interface/Chat.h"
#include "../interface/Colour.h"
#include "../interface/Window_internal.h"
#include "../localisation/Localisation.h"
#include "../management/Finance.h"
#include "../management/NewsItem.h"
#include "../management/Research.h"
#include "../network/network.h"
#include "../network/NetworkUser.h"
#include "../object/Object.h"
#include "../object/ObjectList.h"
#include "../object/ObjectManager.h"
#include "../object/ObjectRepository.h"
#include "../peep/Staff.h"
#include "../platform/platform.h"
#include "../ride/Ride.h"
#include "../ride/RideData.h"
#include "../util/Util.h"
#include "../windows/Intent.h"
#include "../world/Climate.h"
#include "../world/Park.h"
#include "../world/Scenery.h"
#include "../world/Sprite.h"
#include "Viewport.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <deque>
#include <exception>
#include <string>
#include <thread>
#include <vector>

#ifndef NO_TTF
#    include "../drawing/TTF.h"
#endif

using arguments_t = std::vector<std::string>;

static constexpr const char* ClimateNames[] = {
    "cool_and_wet",
    "warm",
    "hot_and_dry",
    "cold",
};

static int32_t console_parse_int(const std::string& src, bool* valid);
static double console_parse_double(const std::string& src, bool* valid);

static void console_write_all_commands(InteractiveConsole& console);
static int32_t cc_variables(InteractiveConsole& console, const arguments_t& argv);
static int32_t cc_windows(InteractiveConsole& console, const arguments_t& argv);
static int32_t cc_help(InteractiveConsole& console, const arguments_t& argv);

static bool invalidArguments(bool* invalid, bool arguments);

#define SET_FLAG(variable, flag, value)                                                                                        \
    {                                                                                                                          \
        if (value)                                                                                                             \
            variable |= flag;                                                                                                  \
        else                                                                                                                   \
            variable &= ~(flag);                                                                                               \
    }

static int32_t console_parse_int(const std::string& src, bool* valid)
{
    utf8* end;
    int32_t value;
    value = static_cast<int32_t>(strtol(src.c_str(), &end, 10));
    *valid = (*end == '\0');
    return value;
}

static double console_parse_double(const std::string& src, bool* valid)
{
    utf8* end;
    double value;
    value = strtod(src.c_str(), &end);
    *valid = (*end == '\0');
    return value;
}

static int32_t cc_clear(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    console.Clear();
    return 0;
}

static int32_t cc_close(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    console.Close();
    return 0;
}

static int32_t cc_hide(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    console.Hide();
    return 0;
}

static int32_t cc_echo(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
        console.WriteLine(argv[0]);
    return 0;
}

static int32_t cc_rides(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
    {
        if (argv[0] == "list")
        {
            for (const auto& ride : GetRideManager())
            {
                auto name = ride.GetName();
                console.WriteFormatLine(
                    "ride: %03d type: %02u subtype %03u operating mode: %02u name: %s", ride.id, ride.type, ride.subtype,
                    ride.mode, name.c_str());
            }
        }
        else if (argv[0] == "set")
        {
            if (argv.size() < 4)
            {
                if (argv.size() > 1 && argv[1] == "mode")
                {
                    console.WriteFormatLine("Ride modes are specified using integer IDs as given below:");
                    for (int32_t i = 0; i < RIDE_MODE_COUNT; i++)
                    {
                        char mode_name[128] = { 0 };
                        rct_string_id mode_string_id = RideModeNames[i];
                        format_string(mode_name, 128, mode_string_id, nullptr);
                        console.WriteFormatLine("%02d - %s", i, mode_name);
                    }
                }
                else
                {
                    console.WriteFormatLine("rides set type <ride id> <ride type>");
                    console.WriteFormatLine("rides set mode [<ride id> <operating mode>]");
                    console.WriteFormatLine("rides set mass <ride id> <mass value>");
                    console.WriteFormatLine("rides set excitement <ride id> <excitement value>");
                    console.WriteFormatLine("rides set intensity <ride id> <intensity value>");
                    console.WriteFormatLine("rides set nausea <ride id> <nausea value>");
                    console.WriteFormatLine("rides set price <ride id / all [type]> <price>");
                }
                return 0;
            }
            if (argv[1] == "type")
            {
                bool int_valid[2] = { false };
                int32_t ride_index = console_parse_int(argv[2], &int_valid[0]);
                int32_t type = console_parse_int(argv[3], &int_valid[1]);
                if (!int_valid[0] || !int_valid[1])
                {
                    console.WriteFormatLine("This command expects integer arguments");
                }
                else if (ride_index < 0)
                {
                    console.WriteFormatLine("Ride index must not be negative");
                }
                else
                {
                    int32_t res = set_operating_setting(ride_index, RideSetSetting::RideType, type);
                    if (res == MONEY32_UNDEFINED)
                    {
                        console.WriteFormatLine("That didn't work");
                    }
                }
            }
            else if (argv[1] == "mode")
            {
                bool int_valid[2] = { false };
                int32_t ride_index = console_parse_int(argv[2], &int_valid[0]);
                int32_t mode = console_parse_int(argv[3], &int_valid[1]);
                if (!int_valid[0] || !int_valid[1])
                {
                    console.WriteFormatLine("This command expects integer arguments");
                }
                else if (ride_index < 0)
                {
                    console.WriteFormatLine("Ride index must not be negative");
                }
                else
                {
                    auto ride = get_ride(ride_index);
                    if (mode <= 0 || mode > (RIDE_MODE_COUNT - 1))
                    {
                        console.WriteFormatLine("Invalid ride mode.");
                    }
                    else if (ride == nullptr)
                    {
                        console.WriteFormatLine("No ride found with index %d", ride_index);
                    }
                    else
                    {
                        ride->mode = static_cast<uint8_t>(mode & 0xFF);
                        invalidate_test_results(ride);
                    }
                }
            }
            else if (argv[1] == "mass")
            {
                bool int_valid[2] = { false };
                int32_t ride_index = console_parse_int(argv[2], &int_valid[0]);
                int32_t mass = console_parse_int(argv[3], &int_valid[1]);

                if (ride_index < 0)
                {
                    console.WriteFormatLine("Ride index must not be negative");
                }
                else if (!int_valid[0] || !int_valid[1])
                {
                    console.WriteFormatLine("This command expects integer arguments");
                }
                else
                {
                    auto ride = get_ride(ride_index);
                    if (mass <= 0)
                    {
                        console.WriteFormatLine("Friction value must be strictly positive");
                    }
                    else if (ride == nullptr)
                    {
                        console.WriteFormatLine("No ride found with index %d", ride_index);
                    }
                    else
                    {
                        for (int32_t i = 0; i < ride->num_vehicles; i++)
                        {
                            uint16_t vehicle_index = ride->vehicles[i];
                            while (vehicle_index != SPRITE_INDEX_NULL)
                            {
                                Vehicle* vehicle = GET_VEHICLE(vehicle_index);
                                vehicle->mass = mass;
                                vehicle_index = vehicle->next_vehicle_on_train;
                            }
                        }
                    }
                }
            }
            else if (argv[1] == "excitement")
            {
                bool int_valid[2] = { false };
                int32_t ride_index = console_parse_int(argv[2], &int_valid[0]);
                ride_rating excitement = console_parse_int(argv[3], &int_valid[1]);

                if (ride_index < 0)
                {
                    console.WriteFormatLine("Ride index must not be negative");
                }
                else if (!int_valid[0] || !int_valid[1])
                {
                    console.WriteFormatLine("This command expects integer arguments");
                }
                else
                {
                    auto ride = get_ride(ride_index);
                    if (excitement <= 0)
                    {
                        console.WriteFormatLine("Excitement value must be strictly positive");
                    }
                    else if (ride == nullptr)
                    {
                        console.WriteFormatLine("No ride found with index %d", ride_index);
                    }
                    else
                    {
                        ride->excitement = excitement;
                    }
                }
            }
            else if (argv[1] == "intensity")
            {
                bool int_valid[2] = { false };
                int32_t ride_index = console_parse_int(argv[2], &int_valid[0]);
                ride_rating intensity = console_parse_int(argv[3], &int_valid[1]);

                if (ride_index < 0)
                {
                    console.WriteFormatLine("Ride index must not be negative");
                }
                else if (!int_valid[0] || !int_valid[1])
                {
                    console.WriteFormatLine("This command expects integer arguments");
                }
                else
                {
                    auto ride = get_ride(ride_index);
                    if (intensity <= 0)
                    {
                        console.WriteFormatLine("Intensity value must be strictly positive");
                    }
                    else if (ride == nullptr)
                    {
                        console.WriteFormatLine("No ride found with index %d", ride_index);
                    }
                    else
                    {
                        ride->intensity = intensity;
                    }
                }
            }
            else if (argv[1] == "nausea")
            {
                bool int_valid[2] = { false };
                int32_t ride_index = console_parse_int(argv[2], &int_valid[0]);
                ride_rating nausea = console_parse_int(argv[3], &int_valid[1]);

                if (ride_index < 0)
                {
                    console.WriteFormatLine("Ride index must not be negative");
                }
                else if (!int_valid[0] || !int_valid[1])
                {
                    console.WriteFormatLine("This command expects integer arguments");
                }
                else
                {
                    auto ride = get_ride(ride_index);
                    if (nausea <= 0)
                    {
                        console.WriteFormatLine("Nausea value must be strictly positive");
                    }
                    else if (ride == nullptr)
                    {
                        console.WriteFormatLine("No ride found with index %d", ride_index);
                    }
                    else
                    {
                        ride->nausea = nausea;
                    }
                }
            }
            else if (argv[1] == "price")
            {
                bool int_valid[2] = { false };
                if (argv[2] == "all")
                {
                    auto arg1 = console_parse_int(argv[3], &int_valid[0]);
                    if (argv.size() <= 4)
                    {
                        auto price = arg1;
                        if (int_valid[0])
                        {
                            for (const auto& ride : GetRideManager())
                            {
                                auto rideSetPrice = RideSetPriceAction(ride.id, price, true);
                                GameActions::Execute(&rideSetPrice);
                            }
                        }
                        else
                        {
                            console.WriteFormatLine("This command expects one or two integer arguments");
                        }
                    }
                    else
                    {
                        auto rideType = arg1;
                        auto price = console_parse_int(argv[4], &int_valid[1]);

                        if (int_valid[0] && int_valid[1])
                        {
                            for (const auto& ride : GetRideManager())
                            {
                                if (ride.type == rideType)
                                {
                                    auto rideSetPrice = RideSetPriceAction(ride.id, price, true);
                                    GameActions::Execute(&rideSetPrice);
                                }
                            }
                        }
                        else
                        {
                            console.WriteFormatLine("This command expects one or two integer arguments");
                        }
                    }
                }
                else
                {
                    int32_t rideId = console_parse_int(argv[2], &int_valid[0]);
                    money16 price = console_parse_int(argv[3], &int_valid[1]);

                    if (!int_valid[0] || !int_valid[1])
                    {
                        console.WriteFormatLine("This command expects the string all or two integer arguments");
                    }
                    else
                    {
                        auto rideSetPrice = RideSetPriceAction(rideId, price, true);
                        GameActions::Execute(&rideSetPrice);
                    }
                }
            }
        }
    }
    else
    {
        console.WriteFormatLine("subcommands: list, set");
    }
    return 0;
}

static int32_t cc_staff(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
    {
        if (argv[0] == "list")
        {
            Peep* peep;
            int32_t i;
            FOR_ALL_STAFF (i, peep)
            {
                auto name = peep->GetName();
                console.WriteFormatLine(
                    "staff id %03d type: %02u energy %03u name %s", i, peep->staff_type, peep->energy, name.c_str());
            }
        }
        else if (argv[0] == "set")
        {
            if (argv.size() < 4)
            {
                console.WriteFormatLine("staff set energy <staff id> <value 0-255>");
                console.WriteFormatLine("staff set costume <staff id> <costume id>");
                for (int32_t i = 0; i < ENTERTAINER_COSTUME_COUNT; i++)
                {
                    char costume_name[128] = { 0 };
                    rct_string_id costume = StaffCostumeNames[i];
                    format_string(costume_name, 128, STR_DROPDOWN_MENU_LABEL, &costume);
                    // That's a terrible hack here. Costume names include inline sprites
                    // that don't work well with the console, so manually skip past them.
                    console.WriteFormatLine("        costume %i: %s", i, costume_name + 7);
                }
                return 0;
            }
            if (argv[1] == "energy")
            {
                int32_t int_val[3];
                bool int_valid[3] = { false };
                int_val[0] = console_parse_int(argv[2], &int_valid[0]);
                int_val[1] = console_parse_int(argv[3], &int_valid[1]);

                if (int_valid[0] && int_valid[1] && ((GET_PEEP(int_val[0])) != nullptr))
                {
                    Peep* peep = GET_PEEP(int_val[0]);

                    peep->energy = int_val[1];
                    peep->energy_target = int_val[1];
                }
            }
            else if (argv[1] == "costume")
            {
                int32_t int_val[2];
                bool int_valid[2] = { false };
                int_val[0] = console_parse_int(argv[2], &int_valid[0]);
                int_val[1] = console_parse_int(argv[3], &int_valid[1]);
                Peep* peep = nullptr;
                if (!int_valid[0])
                {
                    console.WriteLineError("Invalid staff ID");
                    return 1;
                }
                peep = GET_PEEP(int_val[0]);
                bool is_entertainer = peep != nullptr && peep->type == PEEP_TYPE_STAFF
                    && peep->staff_type == STAFF_TYPE_ENTERTAINER;
                if (!is_entertainer)
                {
                    console.WriteLineError("Specified staff is not entertainer");
                    return 1;
                }
                if (!int_valid[1] || int_val[1] < 0 || int_val[1] >= ENTERTAINER_COSTUME_COUNT)
                {
                    console.WriteLineError("Invalid costume ID");
                    return 1;
                }

                uint8_t costume = int_val[1];
                auto staffSetCostumeAction = StaffSetCostumeAction(int_val[0], costume);
                GameActions::Execute(&staffSetCostumeAction);
            }
        }
    }
    else
    {
        console.WriteFormatLine("subcommands: list, set");
    }
    return 0;
}

static int32_t cc_get(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
    {
        if (argv[0] == "park_rating")
        {
            console.WriteFormatLine("park_rating %d", gParkRating);
        }
        else if (argv[0] == "park_value")
        {
            console.WriteFormatLine("park_value %d", gParkValue / 10);
        }
        else if (argv[0] == "company_value")
        {
            console.WriteFormatLine("company_value %d", gCompanyValue / 10);
        }
        else if (argv[0] == "money")
        {
            console.WriteFormatLine("money %d.%d0", gCash / 10, gCash % 10);
        }
        else if (argv[0] == "scenario_initial_cash")
        {
            console.WriteFormatLine("scenario_initial_cash %d", gInitialCash / 10);
        }
        else if (argv[0] == "current_loan")
        {
            console.WriteFormatLine("current_loan %d", gBankLoan / 10);
        }
        else if (argv[0] == "max_loan")
        {
            console.WriteFormatLine("max_loan %d", gMaxBankLoan / 10);
        }
        else if (argv[0] == "guest_initial_cash")
        {
            console.WriteFormatLine("guest_initial_cash %d.%d0", gGuestInitialCash / 10, gGuestInitialCash % 10);
        }
        else if (argv[0] == "guest_initial_happiness")
        {
            uint32_t current_happiness = gGuestInitialHappiness;
            for (int32_t i = 15; i <= 99; i++)
            {
                if (i == 99)
                {
                    console.WriteFormatLine("guest_initial_happiness %d%%  (%d)", 15, gGuestInitialHappiness);
                }
                else if (current_happiness == calculate_guest_initial_happiness(i))
                {
                    console.WriteFormatLine("guest_initial_happiness %d%%  (%d)", i, gGuestInitialHappiness);
                    break;
                }
            }
        }
        else if (argv[0] == "guest_initial_hunger")
        {
            console.WriteFormatLine(
                "guest_initial_hunger %d%%  (%d)", ((255 - gGuestInitialHunger) * 100) / 255, gGuestInitialHunger);
        }
        else if (argv[0] == "guest_initial_thirst")
        {
            console.WriteFormatLine(
                "guest_initial_thirst %d%%  (%d)", ((255 - gGuestInitialThirst) * 100) / 255, gGuestInitialThirst);
        }
        else if (argv[0] == "guest_prefer_less_intense_rides")
        {
            console.WriteFormatLine(
                "guest_prefer_less_intense_rides %d", (gParkFlags & PARK_FLAGS_PREF_LESS_INTENSE_RIDES) != 0);
        }
        else if (argv[0] == "guest_prefer_more_intense_rides")
        {
            console.WriteFormatLine(
                "guest_prefer_more_intense_rides %d", (gParkFlags & PARK_FLAGS_PREF_MORE_INTENSE_RIDES) != 0);
        }
        else if (argv[0] == "forbid_marketing_campaigns")
        {
            console.WriteFormatLine("forbid_marketing_campaigns %d", (gParkFlags & PARK_FLAGS_FORBID_MARKETING_CAMPAIGN) != 0);
        }
        else if (argv[0] == "forbid_landscape_changes")
        {
            console.WriteFormatLine("forbid_landscape_changes %d", (gParkFlags & PARK_FLAGS_FORBID_LANDSCAPE_CHANGES) != 0);
        }
        else if (argv[0] == "forbid_tree_removal")
        {
            console.WriteFormatLine("forbid_tree_removal %d", (gParkFlags & PARK_FLAGS_FORBID_TREE_REMOVAL) != 0);
        }
        else if (argv[0] == "forbid_high_construction")
        {
            console.WriteFormatLine("forbid_high_construction %d", (gParkFlags & PARK_FLAGS_FORBID_HIGH_CONSTRUCTION) != 0);
        }
        else if (argv[0] == "pay_for_rides")
        {
            console.WriteFormatLine("pay_for_rides %d", (gParkFlags & PARK_FLAGS_PARK_FREE_ENTRY) != 0);
        }
        else if (argv[0] == "no_money")
        {
            console.WriteFormatLine("no_money %d", (gParkFlags & PARK_FLAGS_NO_MONEY) != 0);
        }
        else if (argv[0] == "difficult_park_rating")
        {
            console.WriteFormatLine("difficult_park_rating %d", (gParkFlags & PARK_FLAGS_DIFFICULT_PARK_RATING) != 0);
        }
        else if (argv[0] == "difficult_guest_generation")
        {
            console.WriteFormatLine("difficult_guest_generation %d", (gParkFlags & PARK_FLAGS_DIFFICULT_GUEST_GENERATION) != 0);
        }
        else if (argv[0] == "park_open")
        {
            console.WriteFormatLine("park_open %d", (gParkFlags & PARK_FLAGS_PARK_OPEN) != 0);
        }
        else if (argv[0] == "land_rights_cost")
        {
            console.WriteFormatLine("land_rights_cost %d.%d0", gLandPrice / 10, gLandPrice % 10);
        }
        else if (argv[0] == "construction_rights_cost")
        {
            console.WriteFormatLine(
                "construction_rights_cost %d.%d0", gConstructionRightsPrice / 10, gConstructionRightsPrice % 10);
        }
        else if (argv[0] == "climate")
        {
            console.WriteFormatLine("climate %s  (%d)", ClimateNames[gClimate], gClimate);
        }
        else if (argv[0] == "game_speed")
        {
            console.WriteFormatLine("game_speed %d", gGameSpeed);
        }
        else if (argv[0] == "console_small_font")
        {
            console.WriteFormatLine("console_small_font %d", gConfigInterface.console_small_font);
        }
        else if (argv[0] == "location")
        {
            rct_window* w = window_get_main();
            if (w != nullptr)
            {
                int32_t interactionType;
                TileElement* tileElement;
                CoordsXY mapCoord = {};
                rct_viewport* viewport = window_get_viewport(w);
                get_map_coordinates_from_pos(
                    { viewport->view_width / 2, viewport->view_height / 2 }, VIEWPORT_INTERACTION_MASK_TERRAIN, mapCoord,
                    &interactionType, &tileElement, nullptr);

                auto tileMapCoord = TileCoordsXY(mapCoord);
                console.WriteFormatLine("location %d %d", tileMapCoord.x, tileMapCoord.y);
            }
        }
        else if (argv[0] == "window_scale")
        {
            console.WriteFormatLine("window_scale %.3f", gConfigGeneral.window_scale);
        }
        else if (argv[0] == "window_limit")
        {
            console.WriteFormatLine("window_limit %d", gConfigGeneral.window_limit);
        }
        else if (argv[0] == "render_weather_effects")
        {
            console.WriteFormatLine("render_weather_effects %d", gConfigGeneral.render_weather_effects);
        }
        else if (argv[0] == "render_weather_gloom")
        {
            console.WriteFormatLine("render_weather_gloom %d", gConfigGeneral.render_weather_gloom);
        }
        else if (argv[0] == "cheat_sandbox_mode")
        {
            console.WriteFormatLine("cheat_sandbox_mode %d", gCheatsSandboxMode);
        }
        else if (argv[0] == "cheat_disable_clearance_checks")
        {
            console.WriteFormatLine("cheat_disable_clearance_checks %d", gCheatsDisableClearanceChecks);
        }
        else if (argv[0] == "cheat_disable_support_limits")
        {
            console.WriteFormatLine("cheat_disable_support_limits %d", gCheatsDisableSupportLimits);
        }
        else if (argv[0] == "current_rotation")
        {
            console.WriteFormatLine("current_rotation %d", get_current_rotation());
        }
#ifndef NO_TTF
        else if (argv[0] == "enable_hinting")
        {
            console.WriteFormatLine("enable_hinting %d", gConfigFonts.enable_hinting);
        }
#endif
        else
        {
            console.WriteLineWarning("Invalid variable.");
        }
    }
    return 0;
}
static int32_t cc_set(InteractiveConsole& console, const arguments_t& argv)
{
    uint32_t i;
    if (argv.size() > 1)
    {
        int32_t int_val[4];
        bool int_valid[4];
        double double_val[4];
        bool double_valid[4];
        bool invalidArgs = false;

        for (i = 0; i < 4; i++)
        {
            if (i + 1 < argv.size())
            {
                int_val[i] = console_parse_int(argv[i + 1], &int_valid[i]);
                double_val[i] = console_parse_double(argv[i + 1], &double_valid[i]);
            }
            else
            {
                int_val[i] = 0;
                int_valid[i] = false;
                double_val[i] = 0;
                double_valid[i] = false;
            }
        }

        if (argv[0] == "money" && invalidArguments(&invalidArgs, double_valid[0]))
        {
            money32 money = MONEY((int32_t)double_val[0], ((int32_t)(double_val[0] * 100)) % 100);
            if (gCash != money)
            {
                auto setCheatAction = SetCheatAction(CheatType::SetMoney, money);
                setCheatAction.SetCallback([&console](const GameAction*, const GameActionResult* res) {
                    if (res->Error != GA_ERROR::OK)
                        console.WriteLineError("Network error: Permission denied!");
                    else
                        console.Execute("get money");
                });
                GameActions::Execute(&setCheatAction);
            }
            else
            {
                console.Execute("get money");
            }
        }
        else if (argv[0] == "scenario_initial_cash" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gInitialCash = std::clamp(MONEY(int_val[0], 0), MONEY(0, 0), MONEY(1000000, 00));
            console.Execute("get scenario_initial_cash");
        }
        else if (argv[0] == "current_loan" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gBankLoan = std::clamp(MONEY(int_val[0] - (int_val[0] % 1000), 0), MONEY(0, 0), gMaxBankLoan);
            console.Execute("get current_loan");
        }
        else if (argv[0] == "max_loan" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gMaxBankLoan = std::clamp(MONEY(int_val[0] - (int_val[0] % 1000), 0), MONEY(0, 0), MONEY(5000000, 0));
            console.Execute("get max_loan");
        }
        else if (argv[0] == "guest_initial_cash" && invalidArguments(&invalidArgs, double_valid[0]))
        {
            gGuestInitialCash = std::clamp(
                MONEY((int32_t)double_val[0], ((int32_t)(double_val[0] * 100)) % 100), MONEY(0, 0), MONEY(1000, 0));
            console.Execute("get guest_initial_cash");
        }
        else if (argv[0] == "guest_initial_happiness" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gGuestInitialHappiness = calculate_guest_initial_happiness(static_cast<uint8_t>(int_val[0]));
            console.Execute("get guest_initial_happiness");
        }
        else if (argv[0] == "guest_initial_hunger" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gGuestInitialHunger = (std::clamp(int_val[0], 1, 84) * 255 / 100 - 255) * -1;
            console.Execute("get guest_initial_hunger");
        }
        else if (argv[0] == "guest_initial_thirst" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gGuestInitialThirst = (std::clamp(int_val[0], 1, 84) * 255 / 100 - 255) * -1;
            console.Execute("get guest_initial_thirst");
        }
        else if (argv[0] == "guest_prefer_less_intense_rides" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_PREF_LESS_INTENSE_RIDES, int_val[0]);
            console.Execute("get guest_prefer_less_intense_rides");
        }
        else if (argv[0] == "guest_prefer_more_intense_rides" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_PREF_MORE_INTENSE_RIDES, int_val[0]);
            console.Execute("get guest_prefer_more_intense_rides");
        }
        else if (argv[0] == "forbid_marketing_campaigns" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_FORBID_MARKETING_CAMPAIGN, int_val[0]);
            console.Execute("get forbid_marketing_campaigns");
        }
        else if (argv[0] == "forbid_landscape_changes" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_FORBID_LANDSCAPE_CHANGES, int_val[0]);
            console.Execute("get forbid_landscape_changes");
        }
        else if (argv[0] == "forbid_tree_removal" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_FORBID_TREE_REMOVAL, int_val[0]);
            console.Execute("get forbid_tree_removal");
        }
        else if (argv[0] == "forbid_high_construction" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_FORBID_HIGH_CONSTRUCTION, int_val[0]);
            console.Execute("get forbid_high_construction");
        }
        else if (argv[0] == "pay_for_rides" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_PARK_FREE_ENTRY, int_val[0]);
            console.Execute("get pay_for_rides");
        }
        else if (argv[0] == "no_money" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_NO_MONEY, int_val[0]);
            console.Execute("get no_money");
        }
        else if (argv[0] == "difficult_park_rating" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_DIFFICULT_PARK_RATING, int_val[0]);
            console.Execute("get difficult_park_rating");
        }
        else if (argv[0] == "difficult_guest_generation" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_DIFFICULT_GUEST_GENERATION, int_val[0]);
            console.Execute("get difficult_guest_generation");
        }
        else if (argv[0] == "park_open" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            SET_FLAG(gParkFlags, PARK_FLAGS_PARK_OPEN, int_val[0]);
            console.Execute("get park_open");
        }
        else if (argv[0] == "land_rights_cost" && invalidArguments(&invalidArgs, double_valid[0]))
        {
            gLandPrice = std::clamp(
                MONEY((int32_t)double_val[0], ((int32_t)(double_val[0] * 100)) % 100), MONEY(0, 0), MONEY(200, 0));
            console.Execute("get land_rights_cost");
        }
        else if (argv[0] == "construction_rights_cost" && invalidArguments(&invalidArgs, double_valid[0]))
        {
            gConstructionRightsPrice = std::clamp(
                MONEY((int32_t)double_val[0], ((int32_t)(double_val[0] * 100)) % 100), MONEY(0, 0), MONEY(200, 0));
            console.Execute("get construction_rights_cost");
        }
        else if (argv[0] == "climate")
        {
            if (int_valid[0])
            {
                const auto newClimate = int_val[0];
                if (newClimate < 0 || newClimate >= CLIMATE_COUNT)
                {
                    console.WriteLine(language_get_string(STR_INVALID_CLIMATE_ID));
                }
                else
                {
                    auto gameAction = ClimateSetAction(newClimate);
                    GameActions::Execute(&gameAction);
                }
            }
            else
            {
                for (i = 0; i < CLIMATE_COUNT; i++)
                {
                    if (argv[1] == ClimateNames[i])
                    {
                        gClimate = i;
                        break;
                    }
                }
            }

            if (i == CLIMATE_COUNT)
                invalidArgs = true;
            else
                console.Execute("get climate");
        }
        else if (argv[0] == "game_speed" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gGameSpeed = std::clamp(int_val[0], 1, 8);
            console.Execute("get game_speed");
        }
        else if (argv[0] == "console_small_font" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gConfigInterface.console_small_font = (int_val[0] != 0);
            config_save_default();
            console.Execute("get console_small_font");
        }
        else if (argv[0] == "location" && invalidArguments(&invalidArgs, int_valid[0] && int_valid[1]))
        {
            rct_window* w = window_get_main();
            if (w != nullptr)
            {
                auto location = TileCoordsXYZ(int_val[0], int_val[1], 0).ToCoordsXYZ().ToTileCentre();
                location.z = tile_element_height(location);
                w->SetLocation(location.x, location.y, location.z);
                viewport_update_position(w);
                console.Execute("get location");
            }
        }
        else if (argv[0] == "window_scale" && invalidArguments(&invalidArgs, double_valid[0]))
        {
            float newScale = static_cast<float>(0.001 * std::trunc(1000 * double_val[0]));
            gConfigGeneral.window_scale = std::clamp(newScale, 0.5f, 5.0f);
            config_save_default();
            gfx_invalidate_screen();
            context_trigger_resize();
            context_update_cursor_scale();
            console.Execute("get window_scale");
        }
        else if (argv[0] == "window_limit" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            window_set_window_limit(int_val[0]);
            console.Execute("get window_limit");
        }
        else if (argv[0] == "render_weather_effects" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gConfigGeneral.render_weather_effects = (int_val[0] != 0);
            config_save_default();
            console.Execute("get render_weather_effects");
        }
        else if (argv[0] == "render_weather_gloom" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gConfigGeneral.render_weather_gloom = (int_val[0] != 0);
            config_save_default();
            console.Execute("get render_weather_gloom");
        }
        else if (argv[0] == "cheat_sandbox_mode" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            if (gCheatsSandboxMode != (int_val[0] != 0))
            {
                auto setCheatAction = SetCheatAction(CheatType::SandboxMode, int_val[0] != 0);
                setCheatAction.SetCallback([&console](const GameAction*, const GameActionResult* res) {
                    if (res->Error != GA_ERROR::OK)
                        console.WriteLineError("Network error: Permission denied!");
                    else
                        console.Execute("get cheat_sandbox_mode");
                });
                GameActions::Execute(&setCheatAction);
            }
            else
            {
                console.Execute("get cheat_sandbox_mode");
            }
        }
        else if (argv[0] == "cheat_disable_clearance_checks" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            if (gCheatsDisableClearanceChecks != (int_val[0] != 0))
            {
                auto setCheatAction = SetCheatAction(CheatType::DisableClearanceChecks, int_val[0] != 0);
                setCheatAction.SetCallback([&console](const GameAction*, const GameActionResult* res) {
                    if (res->Error != GA_ERROR::OK)
                        console.WriteLineError("Network error: Permission denied!");
                    else
                        console.Execute("get cheat_disable_clearance_checks");
                });
                GameActions::Execute(&setCheatAction);
            }
            else
            {
                console.Execute("get cheat_disable_clearance_checks");
            }
        }
        else if (argv[0] == "cheat_disable_support_limits" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            if (gCheatsDisableSupportLimits != (int_val[0] != 0))
            {
                auto setCheatAction = SetCheatAction(CheatType::DisableSupportLimits, int_val[0] != 0);
                setCheatAction.SetCallback([&console](const GameAction*, const GameActionResult* res) {
                    if (res->Error != GA_ERROR::OK)
                        console.WriteLineError("Network error: Permission denied!");
                    else
                        console.Execute("get cheat_disable_support_limits");
                });
                GameActions::Execute(&setCheatAction);
            }
            else
            {
                console.Execute("get cheat_disable_support_limits");
            }
        }
        else if (argv[0] == "current_rotation" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            uint8_t currentRotation = get_current_rotation();
            rct_window* mainWindow = window_get_main();
            int32_t newRotation = int_val[0];
            if (newRotation < 0 || newRotation > 3)
            {
                console.WriteLineError("Invalid argument. Valid rotations are 0-3.");
            }
            else if (newRotation != currentRotation && mainWindow != nullptr)
            {
                window_rotate_camera(mainWindow, newRotation - currentRotation);
            }
            console.Execute("get current_rotation");
        }
#ifndef NO_TTF
        else if (argv[0] == "enable_hinting" && invalidArguments(&invalidArgs, int_valid[0]))
        {
            gConfigFonts.enable_hinting = (int_val[0] != 0);
            config_save_default();
            console.Execute("get enable_hinting");
            ttf_toggle_hinting();
        }
#endif
        else if (invalidArgs)
        {
            console.WriteLineError("Invalid arguments.");
        }
        else
        {
            console.WriteLineError("Invalid variable.");
        }

        gfx_invalidate_screen();
    }
    else
    {
        console.WriteLineError("Value required.");
    }
    return 0;
}

static int32_t cc_twitch([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
#ifdef DISABLE_TWITCH
    console.WriteLineError("OpenRCT2 build not compiled with Twitch integration.");
#else
    // TODO: Add some twitch commands
    // Display a message to the player for now
    console.WriteLine("To be implemented");
#endif
    return 0;
}

static int32_t cc_load_object(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
    {
        char name[9] = { 0 };
        std::fill_n(name, 8, ' ');
        int32_t i = 0;
        for (const char* ch = argv[0].c_str(); *ch != '\0' && i < 8; ch++)
        {
            name[i++] = *ch;
        }

        const ObjectRepositoryItem* ori = object_repository_find_object_by_name(name);
        if (ori == nullptr)
        {
            console.WriteLineError("Could not find the object.");
            return 1;
        }

        const rct_object_entry* entry = &ori->ObjectEntry;
        void* loadedObject = object_manager_get_loaded_object(entry);
        if (loadedObject != nullptr)
        {
            console.WriteLineError("Object is already in scenario.");
            return 1;
        }

        loadedObject = object_manager_load_object(entry);
        if (loadedObject == nullptr)
        {
            console.WriteLineError("Unable to load object.");
            return 1;
        }
        auto groupIndex = object_manager_get_loaded_object_entry_index(loadedObject);

        uint8_t objectType = entry->GetType();
        if (objectType == OBJECT_TYPE_RIDE)
        {
            // Automatically research the ride so it's supported by the game.
            rct_ride_entry* rideEntry;
            int32_t rideType;

            rideEntry = get_ride_entry(groupIndex);

            for (int32_t j = 0; j < MAX_RIDE_TYPES_PER_RIDE_ENTRY; j++)
            {
                rideType = rideEntry->ride_type[j];
                research_insert_ride_entry(rideType, groupIndex, rideEntry->category[0], true);
            }

            gSilentResearch = true;
            research_reset_current_item();
            gSilentResearch = false;
        }
        else if (objectType == OBJECT_TYPE_SCENERY_GROUP)
        {
            research_insert_scenery_group_entry(groupIndex, true);

            gSilentResearch = true;
            research_reset_current_item();
            gSilentResearch = false;
        }
        scenery_set_default_placement_configuration();

        auto intent = Intent(INTENT_ACTION_REFRESH_NEW_RIDES);
        context_broadcast_intent(&intent);

        gWindowUpdateTicks = 0;
        gfx_invalidate_screen();
        console.WriteLine("Object file loaded.");
    }

    return 0;
}

static int32_t cc_object_count(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    const utf8* object_type_names[] = {
        "Rides", "Small scenery",  "Large scenery",  "Walls",          "Banners",
        "Paths", "Path Additions", "Scenery groups", "Park entrances", "Water",
    };

    for (int32_t i = 0; i < 10; i++)
    {
        int32_t entryGroupIndex = 0;
        for (; entryGroupIndex < object_entry_group_counts[i]; entryGroupIndex++)
        {
            if (object_entry_get_chunk(i, entryGroupIndex) == nullptr)
            {
                break;
            }
        }
        console.WriteFormatLine("%s: %d/%d", object_type_names[i], entryGroupIndex, object_entry_group_counts[i]);
    }

    return 0;
}

static int32_t cc_open(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
    {
        bool title = (gScreenFlags & SCREEN_FLAGS_TITLE_DEMO) != 0;
        bool invalidTitle = false;
        if (argv[0] == "object_selection" && invalidArguments(&invalidTitle, !title))
        {
            // Only this window should be open for safety reasons
            window_close_all();
            context_open_window(WC_EDITOR_OBJECT_SELECTION);
        }
        else if (argv[0] == "inventions_list" && invalidArguments(&invalidTitle, !title))
        {
            context_open_window(WC_EDITOR_INVENTION_LIST);
        }
        else if (argv[0] == "scenario_options" && invalidArguments(&invalidTitle, !title))
        {
            context_open_window(WC_EDITOR_SCENARIO_OPTIONS);
        }
        else if (argv[0] == "options")
        {
            context_open_window(WC_OPTIONS);
        }
        else if (argv[0] == "themes")
        {
            context_open_window(WC_THEMES);
        }
        else if (argv[0] == "title_sequences")
        {
            context_open_window(WC_TITLE_EDITOR);
        }
        else if (invalidTitle)
        {
            console.WriteLineError("Cannot open this window in the title screen.");
        }
        else
        {
            console.WriteLineError("Invalid window.");
        }
    }
    return 0;
}

static int32_t cc_remove_unused_objects(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    int32_t result = editor_remove_unused_objects();
    console.WriteFormatLine("%d unused object entries have been removed.", result);
    return 0;
}

static int32_t cc_remove_floating_objects(InteractiveConsole& console, const arguments_t& argv)
{
    uint16_t result = remove_floating_sprites();
    console.WriteFormatLine("Removed %d flying objects", result);
    return 0;
}

static int32_t cc_remove_park_fences(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    tile_element_iterator it;
    tile_element_iterator_begin(&it);
    do
    {
        if (it.element->GetType() == TILE_ELEMENT_TYPE_SURFACE)
        {
            // Remove all park fence flags
            it.element->AsSurface()->SetParkFences(0);
        }
    } while (tile_element_iterator_next(&it));

    gfx_invalidate_screen();

    console.WriteFormatLine("Park fences have been removed.");
    return 0;
}

static int32_t cc_show_limits(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    map_reorganise_elements();
    int32_t tileElementCount = gNextFreeTileElement - gTileElements - 1;

    int32_t rideCount = ride_get_count();
    int32_t spriteCount = 0;
    for (int32_t i = 1; i < SPRITE_LIST_COUNT; ++i)
    {
        spriteCount += gSpriteListCount[i];
    }

    int32_t staffCount = 0;
    for (int32_t i = 0; i < STAFF_MAX_COUNT; ++i)
    {
        if (gStaffModes[i] & 1)
        {
            staffCount++;
        }
    }

    int32_t bannerCount = 0;
    for (BannerIndex i = 0; i < MAX_BANNERS; ++i)
    {
        auto banner = GetBanner(i);
        if (banner->type != BANNER_NULL)
        {
            bannerCount++;
        }
    }

    console.WriteFormatLine("Sprites: %d/%d", spriteCount, MAX_SPRITES);
    console.WriteFormatLine("Map Elements: %d/%d", tileElementCount, MAX_TILE_ELEMENTS);
    console.WriteFormatLine("Banners: %d/%zu", bannerCount, MAX_BANNERS);
    console.WriteFormatLine("Rides: %d/%d", rideCount, MAX_RIDES);
    console.WriteFormatLine("Staff: %d/%d", staffCount, STAFF_MAX_COUNT);
    console.WriteFormatLine("Images: %zu/%zu", ImageListGetUsedCount(), ImageListGetMaximum());
    return 0;
}

static int32_t cc_for_date([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    if (argv.size() < 1 || argv.size() > 3)
    {
        return -1;
    }

    // All cases involve providing a year, so grab that first
    year = atoi(argv[0].c_str());
    if (year < 1 || year > MAX_YEAR)
    {
        return -1;
    }

    // YYYY (no month provided, preserve existing month)
    if (argv.size() == 1)
    {
        month = gDateMonthsElapsed % MONTH_COUNT + 1;
    }

    // YYYY MM or YYYY MM DD (month provided)
    if (argv.size() >= 2)
    {
        month = atoi(argv[1].c_str());
        month -= 2;
        if (month < 1 || month > MONTH_COUNT)
        {
            return -1;
        }
    }

    // YYYY OR YYYY MM (no day provided, preserve existing day)
    if (argv.size() <= 2)
    {
        day = std::clamp(
            gDateMonthTicks / (0x10000 / days_in_month[month - 1]) + 1, 1, static_cast<int>(days_in_month[month - 1]));
    }

    // YYYY MM DD (year, month, and day provided)
    if (argv.size() == 3)
    {
        day = atoi(argv[2].c_str());
        if (day < 1 || day > days_in_month[month - 1])
        {
            return -1;
        }
    }

    date_set(year, month, day);
    window_invalidate_by_class(WC_BOTTOM_TOOLBAR);

    return 1;
}

static int32_t cc_load_park([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    if (argv.size() < 1)
    {
        console.WriteLine("Parameters required <filename>");
        return 0;
    }

    char savePath[MAX_PATH];
    if (String::IndexOf(argv[0].c_str(), '/') == SIZE_MAX && String::IndexOf(argv[0].c_str(), '\\') == SIZE_MAX)
    {
        // no / or \ was included. File should be in save dir.
        platform_get_user_directory(savePath, "save", sizeof(savePath));
        safe_strcat_path(savePath, argv[0].c_str(), sizeof(savePath));
    }
    else
    {
        safe_strcpy(savePath, argv[0].c_str(), sizeof(savePath));
    }
    if (!String::EndsWith(savePath, ".sv6", true) && !String::EndsWith(savePath, ".sc6", true))
    {
        path_append_extension(savePath, ".sv6", sizeof(savePath));
    }
    if (context_load_park_from_file(savePath))
    {
        console.WriteFormatLine("Park %s was loaded successfully", savePath);
    }
    else
    {
        console.WriteFormatLine("Loading Park %s failed", savePath);
    }
    return 1;
}

static int32_t cc_save_park([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    if (argv.size() < 1)
    {
        save_game_cmd();
    }
    else
    {
        save_game_cmd(argv[0].c_str());
    }
    return 1;
}

static int32_t cc_say(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() == NETWORK_MODE_NONE || network_get_status() != NETWORK_STATUS_CONNECTED
        || network_get_authstatus() != NETWORK_AUTH_OK)
    {
        console.WriteFormatLine("This command only works in multiplayer mode.");
        return 0;
    }
    else
    {
        if (!argv.empty())
        {
            network_send_chat(argv[0].c_str());
            return 1;
        }
        else
        {
            console.WriteFormatLine("Input your message");
            return 0;
        }
    }
}

static int32_t cc_replay_startrecord(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() != NETWORK_MODE_NONE)
    {
        console.WriteFormatLine("This command is currently not supported in multiplayer mode.");
        return 0;
    }

    if (argv.size() < 1)
    {
        console.WriteFormatLine("Parameters required <replay_name> [<max_ticks = 0xFFFFFFFF>]");
        return 0;
    }

    std::string name = argv[0];

    if (!String::EndsWith(name, ".sv6r", true))
    {
        name += ".sv6r";
    }
    std::string outPath = OpenRCT2::GetContext()->GetPlatformEnvironment()->GetDirectoryPath(
        OpenRCT2::DIRBASE::USER, OpenRCT2::DIRID::REPLAY);
    name = Path::Combine(outPath, name);

    // If ticks are specified by user use that otherwise maximum ticks specified by const.
    uint32_t maxTicks = OpenRCT2::k_MaxReplayTicks;
    if (argv.size() >= 2)
    {
        maxTicks = atol(argv[1].c_str());
    }

    auto* replayManager = OpenRCT2::GetContext()->GetReplayManager();
    if (replayManager->StartRecording(name, maxTicks))
    {
        OpenRCT2::ReplayRecordInfo info;
        replayManager->GetCurrentReplayInfo(info);

        const char* logFmt = "Replay recording started: (%s) %s";
        console.WriteFormatLine(logFmt, info.Name.c_str(), info.FilePath.c_str());
        log_info(logFmt, info.Name.c_str(), info.FilePath.c_str());

        return 1;
    }

    return 0;
}

static int32_t cc_replay_stoprecord(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() != NETWORK_MODE_NONE)
    {
        console.WriteFormatLine("This command is currently not supported in multiplayer mode.");
        return 0;
    }

    auto* replayManager = OpenRCT2::GetContext()->GetReplayManager();
    if (!replayManager->IsRecording() && !replayManager->IsNormalising())
    {
        console.WriteFormatLine("Replay currently not recording");
        return 0;
    }

    OpenRCT2::ReplayRecordInfo info;
    replayManager->GetCurrentReplayInfo(info);

    if (replayManager->StopRecording())
    {
        const char* logFmt = "Replay recording stopped: (%s) %s\n"
                             "  Ticks: %u\n"
                             "  Commands: %u\n"
                             "  Checksums: %u";

        console.WriteFormatLine(
            logFmt, info.Name.c_str(), info.FilePath.c_str(), info.Ticks, info.NumCommands, info.NumChecksums);
        log_info(logFmt, info.Name.c_str(), info.FilePath.c_str(), info.Ticks, info.NumCommands, info.NumChecksums);

        return 1;
    }

    return 0;
}

static int32_t cc_replay_start(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() != NETWORK_MODE_NONE)
    {
        console.WriteFormatLine("This command is currently not supported in multiplayer mode.");
        return 0;
    }

    if (argv.size() < 1)
    {
        console.WriteFormatLine("Parameters required <replay_name>");
        return 0;
    }

    std::string name = argv[0];

    auto* replayManager = OpenRCT2::GetContext()->GetReplayManager();
    if (replayManager->StartPlayback(name))
    {
        OpenRCT2::ReplayRecordInfo info;
        replayManager->GetCurrentReplayInfo(info);

        std::time_t ts = info.TimeRecorded;

        char recordingDate[128] = {};
        std::strftime(recordingDate, sizeof(recordingDate), "%c", std::localtime(&ts));

        const char* logFmt = "Replay playback started: %s\n"
                             "  Date Recorded: %s\n"
                             "  Ticks: %u\n"
                             "  Commands: %u\n"
                             "  Checksums: %u";

        console.WriteFormatLine(logFmt, info.FilePath.c_str(), recordingDate, info.Ticks, info.NumCommands, info.NumChecksums);
        log_info(logFmt, info.FilePath.c_str(), recordingDate, info.Ticks, info.NumCommands, info.NumChecksums);

        return 1;
    }

    return 0;
}

static int32_t cc_replay_stop(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() != NETWORK_MODE_NONE)
    {
        console.WriteFormatLine("This command is currently not supported in multiplayer mode.");
        return 0;
    }

    auto* replayManager = OpenRCT2::GetContext()->GetReplayManager();
    if (replayManager->StopPlayback())
    {
        console.WriteFormatLine("Stopped replay");
        return 1;
    }

    return 0;
}

static int32_t cc_replay_normalise(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() != NETWORK_MODE_NONE)
    {
        console.WriteFormatLine("This command is currently not supported in multiplayer mode.");
        return 0;
    }

    if (argv.size() < 2)
    {
        console.WriteFormatLine("Parameters required <replay_input> <replay_output>");
        return 0;
    }

    std::string inputFile = argv[0];
    std::string outputFile = argv[1];

    if (!String::EndsWith(outputFile, ".sv6r", true))
    {
        outputFile += ".sv6r";
    }
    std::string outPath = OpenRCT2::GetContext()->GetPlatformEnvironment()->GetDirectoryPath(
        OpenRCT2::DIRBASE::USER, OpenRCT2::DIRID::REPLAY);
    outputFile = Path::Combine(outPath, outputFile);

    auto* replayManager = OpenRCT2::GetContext()->GetReplayManager();
    if (replayManager->NormaliseReplay(inputFile, outputFile))
    {
        console.WriteFormatLine("Stopped replay");
        return 1;
    }

    return 0;
}

static int32_t cc_mp_desync(InteractiveConsole& console, const arguments_t& argv)
{
    int32_t desyncType = 0;
    if (argv.size() >= 1)
    {
        desyncType = atoi(argv[0].c_str());
    }

    std::vector<Peep*> peeps;

    for (int i = 0; i < MAX_SPRITES; i++)
    {
        rct_sprite* sprite = get_sprite(i);
        if (sprite->generic.sprite_identifier == SPRITE_IDENTIFIER_NULL)
            continue;

        auto peep = sprite->AsPeep();
        if (peep != nullptr)
            peeps.push_back(peep);
    }

    switch (desyncType)
    {
        case 0: // Peep t-shirts.
        {
            if (peeps.empty())
            {
                console.WriteFormatLine("No peeps");
            }
            else
            {
                auto* peep = peeps[0];
                if (peeps.size() > 1)
                    peep = peeps[util_rand() % peeps.size() - 1];
                peep->tshirt_colour = util_rand() & 0xFF;
                invalidate_sprite_0(peep);
            }
            break;
        }
        case 1: // Remove random peep.
        {
            if (peeps.empty())
            {
                console.WriteFormatLine("No peep removed");
            }
            else
            {
                auto* peep = peeps[0];
                if (peeps.size() > 1)
                    peep = peeps[util_rand() % peeps.size() - 1];
                peep->Remove();
            }
            break;
        }
    }
    return 0;
}

#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
static int32_t cc_abort([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    std::abort();
    return 0;
}

static int32_t cc_dereference([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    // Dereference a nullptr to induce a crash to be caught by crash handler, on supported platforms
    uint8_t* myptr = nullptr;
    *myptr = 42;
    return 0;
#pragma GCC diagnostic pop
}

static int32_t cc_terminate([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    std::terminate();
    return 0;
}
#pragma warning(pop)

static int32_t cc_assert([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    if (!argv.empty())
        Guard::Assert(false, "%s", argv[0].c_str());
    else
        Guard::Assert(false);
    return 0;
}

static int32_t cc_add_news_item([[maybe_unused]] InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    printf("argv.size() = %zu\n", argv.size());
    if (argv.size() < 3)
    {
        console.WriteLineWarning("Too few arguments");
        static_assert(NEWS_ITEM_TYPE_COUNT == 10, "NEWS_ITEM_TYPE_COUNT changed, update console command!");
        console.WriteLine("add_news_item <type> <message> <assoc>");
        console.WriteLine("type is one of:");
        console.WriteLine("    0 (NEWS_ITEM_NULL)");
        console.WriteLine("    1 (NEWS_ITEM_RIDE)");
        console.WriteLine("    2 (NEWS_ITEM_PEEP_ON_RIDE)");
        console.WriteLine("    3 (NEWS_ITEM_PEEP)");
        console.WriteLine("    4 (NEWS_ITEM_MONEY)");
        console.WriteLine("    5 (NEWS_ITEM_BLANK)");
        console.WriteLine("    6 (NEWS_ITEM_RESEARCH)");
        console.WriteLine("    7 (NEWS_ITEM_PEEPS)");
        console.WriteLine("    8 (NEWS_ITEM_AWARD)");
        console.WriteLine("    9 (NEWS_ITEM_GRAPH)");
        console.WriteLine("message is the message to display, wrapped in quotes for multiple words");
        console.WriteLine("assoc is the associated id of ride/peep/tile/etc.");
        return 1;
    }
    auto type = atoi(argv[0].c_str());
    auto msg = argv[1].c_str();
    auto assoc = atoi(argv[2].c_str());
    news_item_add_to_queue_raw(type, msg, assoc);
    return 0;
}

static int32_t cc_users(InteractiveConsole& console, const arguments_t& argv)
{
    if (network_get_mode() != NETWORK_MODE_SERVER)
    {
        console.WriteFormatLine("This command is only supported in server-side multiplayer.");
        return 0;
    }
    if (!argv.empty())
    {
        NetworkUserManager userManager;
        if (argv[0] == "list")
        {
            userManager.Load();
            std::vector<std::string> userHashes = userManager.GetAllUserHashes();
            for (size_t i = 0; i < userHashes.size(); i++)
            {
                const NetworkUser* networkUser = userManager.GetUserByHash(userHashes[i]);
                if (networkUser != nullptr)
                {
                    console.WriteFormatLine("User %u", i);
                    console.WriteFormatLine("  Hash: %s", networkUser->Hash.c_str());
                    console.WriteFormatLine("  Name: %s", networkUser->Name.c_str());
                    // FIXME: Need to handle null GroupId
                    console.WriteFormatLine("  GID: %u", networkUser->GroupId.GetValue());
                }
            }
        }
        else if (argv[0] == "set")
        {
            if (argv.size() < 3)
            {
                console.WriteFormatLine("users set <username> <group id>");
            }
            else
            {
                std::string username = argv[1];
                // FIXME: Handle errors in passed GroupId (atoi returns 0 if error)
                uint8_t gid = atoi(argv[2].c_str());
                userManager.Load();
                const NetworkUser* networkUser = userManager.GetUserByName(username);
                if (networkUser != nullptr)
                {
                    uint8_t oldGid = networkUser->GroupId.GetValue();
//                    networkUser->GroupId = gid;
                    console.WriteFormatLine("Changed %s from %u to %u", networkUser->Name.c_str(), oldGid, gid);
                }
                else
                {
                    console.WriteFormatLine("No user %s found", username.c_str());
                }
            }
        }
        else if (argv[0] == "get")
        {
            if (argv.size() < 2)
            {
                console.WriteLine("users get <username>");
            }
            else
            {
                std::string username = argv[1];
                userManager.Load();
                const NetworkUser* networkUser = userManager.GetUserByName(username);
                if (networkUser != nullptr)
                {
                    console.WriteLine("Found user");
                    console.WriteFormatLine("  Hash: %s", networkUser->Hash.c_str());
                    console.WriteFormatLine("  Name: %s", networkUser->Name.c_str());
                    // FIXME: Need to handle null GroupId
                    console.WriteFormatLine("  GID: %u", networkUser->GroupId.GetValue());
                }
                else
                {
                    console.WriteFormatLine("No user %s found", username.c_str());
                }
            }
        }
    }
    else
    {
        console.WriteLine("subcommands: list, set, get");
    }
    return 0;
}

using console_command_func = int32_t (*)(InteractiveConsole& console, const arguments_t& argv);
struct console_command
{
    const utf8* command;
    console_command_func func;
    const utf8* help;
    const utf8* usage;
};

// clang-format off
static constexpr const utf8* console_variable_table[] = {
    "park_rating",
    "park_value",
    "company_value",
    "money",
    "scenario_initial_cash",
    "current_loan",
    "max_loan",
    "guest_initial_cash",
    "guest_initial_happiness",
    "guest_initial_hunger",
    "guest_initial_thirst",
    "guest_prefer_less_intense_rides",
    "guest_prefer_more_intense_rides",
    "forbid_marketing_campaigns",
    "forbid_landscape_changes",
    "forbid_tree_removal",
    "forbid_high_construction",
    "pay_for_rides",
    "no_money",
    "difficult_park_rating",
    "difficult_guest_generation",
    "land_rights_cost",
    "construction_rights_cost",
    "park_open",
    "climate",
    "game_speed",
    "console_small_font",
    "location",
    "window_scale",
    "window_limit",
    "render_weather_effects",
    "render_weather_gloom",
    "cheat_sandbox_mode",
    "cheat_disable_clearance_checks",
    "cheat_disable_support_limits",
    "current_rotation",
};
static constexpr const utf8* console_window_table[] = {
    "object_selection",
    "inventions_list",
    "scenario_options",
    "options",
    "themes",
    "title_sequences"
};

static constexpr const console_command console_command_table[] = {
    { "abort", cc_abort, "Calls std::abort(), for testing purposes only.", "abort" },
    { "add_news_item", cc_add_news_item, "Inserts a news item", "add_news_item [<type> <message> <assoc>]" },
    { "assert", cc_assert, "Triggers assertion failure, for testing purposes only", "assert" },
    { "clear", cc_clear, "Clears the console.", "clear" },
    { "close", cc_close, "Closes the console.", "close" },
    { "date", cc_for_date, "Sets the date to a given date.", "Format <year>[ <month>[ <day>]]." },
    { "dereference", cc_dereference, "Dereferences a nullptr, for testing purposes only", "dereference" },
    { "echo", cc_echo, "Echoes the text to the console.", "echo <text>" },
    { "exit", cc_close, "Closes the console.", "exit" },
    { "get", cc_get, "Gets the value of the specified variable.", "get <variable>" },
    { "help", cc_help, "Lists commands or info about a command.", "help [command]" },
    { "hide", cc_hide, "Hides the console.", "hide" },
    { "load_object", cc_load_object, "Loads the object file into the scenario.\n"
                                    "Loading a scenery group will not load its associated objects.\n"
                                    "This is a safer method opposed to \"open object_selection\".",
                                    "load_object <objectfilenodat>" },
    { "load_park", cc_load_park, "Load park from save directory or by absolute path", "load_park <filename>" },
    { "object_count", cc_object_count, "Shows the number of objects of each type in the scenario.", "object_count" },
    { "open", cc_open, "Opens the window with the give name.", "open <window>." },
    { "quit", cc_close, "Closes the console.", "quit" },
    { "remove_park_fences", cc_remove_park_fences, "Removes all park fences from the surface", "remove_park_fences" },
    { "remove_unused_objects", cc_remove_unused_objects, "Removes all the unused objects from the object selection.", "remove_unused_objects" },
    { "remove_floating_objects", cc_remove_floating_objects, "Removes floating objects", "remove_floating_objects"},
    { "rides", cc_rides, "Ride management.", "rides <subcommand>" },
    { "save_park", cc_save_park, "Save current state of park. If no name specified default path will be used.", "save_park [name]" },
    { "say", cc_say, "Say to other players.", "say <message>" },
    { "set", cc_set, "Sets the variable to the specified value.", "set <variable> <value>" },
    { "show_limits", cc_show_limits, "Shows the map data counts and limits.", "show_limits" },
    { "staff", cc_staff, "Staff management.", "staff <subcommand>" },
    { "terminate", cc_terminate, "Calls std::terminate(), for testing purposes only.", "terminate" },
    { "twitch", cc_twitch, "Twitch API", "twitch" },
    { "variables", cc_variables, "Lists all the variables that can be used with get and sometimes set.", "variables" },
    { "windows", cc_windows, "Lists all the windows that can be opened.", "windows" },
    { "replay_startrecord", cc_replay_startrecord, "Starts recording a new replay.", "replay_startrecord <name> [max_ticks]"},
    { "replay_stoprecord", cc_replay_stoprecord, "Stops recording a new replay.", "replay_stoprecord"},
    { "replay_start", cc_replay_start, "Starts a replay", "replay_start <name>"},
    { "replay_stop", cc_replay_stop, "Stops the replay", "replay_stop"},
    { "replay_normalise", cc_replay_normalise, "Normalises the replay to remove all gaps", "replay_normalise <input file> <output file>"},
    { "mp_desync", cc_mp_desync, "Forces a multiplayer desync", "cc_mp_desync [desync_type, 0 = Random t-shirt color on random peep, 1 = Remove random peep ]"},
    { "users", cc_users, "Control multiplayer user permissions", "users <list|edit> [args]"},

};
// clang-format on

static int32_t cc_windows(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    for (auto s : console_window_table)
    {
        console.WriteLine(s);
    }
    return 0;
}

static int32_t cc_variables(InteractiveConsole& console, [[maybe_unused]] const arguments_t& argv)
{
    for (auto s : console_variable_table)
    {
        console.WriteLine(s);
    }
    return 0;
}

static int32_t cc_help(InteractiveConsole& console, const arguments_t& argv)
{
    if (!argv.empty())
    {
        for (const auto& c : console_command_table)
        {
            if (argv[0] == c.command)
            {
                console.WriteLine(c.help);
                console.WriteFormatLine("\nUsage:   %s", c.usage);
            }
        }
    }
    else
    {
        console_write_all_commands(console);
    }
    return 0;
}

static void console_write_all_commands(InteractiveConsole& console)
{
    for (const auto& c : console_command_table)
    {
        console.WriteLine(c.command);
    }
}

static bool invalidArguments(bool* invalid, bool arguments)
{
    if (!arguments)
    {
        *invalid = true;
        return false;
    }
    return true;
}

void InteractiveConsole::Execute(const std::string& s)
{
    arguments_t argv;
    argv.reserve(8);

    const utf8* start = s.c_str();
    const utf8* end;
    bool inQuotes = false;
    do
    {
        while (*start == ' ')
            start++;

        if (*start == '"')
        {
            inQuotes = true;
            start++;
        }
        else
        {
            inQuotes = false;
        }

        end = start;
        while (*end != 0)
        {
            if (*end == ' ' && !inQuotes)
                break;
            if (*end == '"' && inQuotes)
                break;
            end++;
        }
        size_t length = end - start;

        if (length > 0)
        {
            argv.emplace_back(start, length);
        }

        start = end;
    } while (*end != 0);

    if (argv.empty())
        return;

    bool validCommand = false;

    for (const auto& c : console_command_table)
    {
        if (argv[0] == c.command)
        {
            argv.erase(argv.begin());
            c.func(*this, argv);
            validCommand = true;
            break;
        }
    }

    if (!validCommand)
    {
        WriteLineError("Unknown command. Type help to list available commands.");
    }
}

void InteractiveConsole::WriteLine(const std::string& s)
{
    WriteLine(s, FORMAT_WINDOW_COLOUR_2);
}

void InteractiveConsole::WriteLineError(const std::string& s)
{
    WriteLine(s, FORMAT_RED);
}

void InteractiveConsole::WriteLineWarning(const std::string& s)
{
    WriteLine(s, FORMAT_YELLOW);
}

void InteractiveConsole::WriteFormatLine(const char* format, ...)
{
    va_list list;
    va_start(list, format);
    auto buffer = String::Format_VA(format, list);
    va_end(list);

    auto s = std::string(buffer);
    std::free(buffer);
    WriteLine(s);
}
