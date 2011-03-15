
/*
 * ntsczones.h
 * Copyright (C) 2011 Einar Rünkaru <einarry at smail dot ee>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define TIMEZONE_NAME "/etc/timezone"
#define LOCALTIME_LINK "/etc/localtime"
#define ZONEINFO_STR   "zoneinfo/"

/*
 * Linux timezones where NTSC standard is used
 * Based on info gathered from wikipedia
 */
const char *ntsc_zones[] = {
// Antigua & Barbuda
	"America/Antigua",
// Aruba
	"America/Aruba",
// Bahamas
	"America/Nassau",
// Barbados
	"America/Barbados",
// Belize
	"America/Belize",
// Bermuda
	"Atlantic/Bermuda",
// Bolivia
	"America/La_Paz",
// Burma
	"Asia/Rangoon",
// Canada
	"America/St_Johns",
	"America/Halifax",
	"America/Glace_Bay",
	"America/Moncton",
	"America/Goose_Bay",
	"America/Blanc-Sablon",
	"America/Montreal",
	"America/Toronto",
	"America/Nipigon",
	"America/Thunder_Bay",
	"America/Iqaluit",
	"America/Pangnirtung",
	"America/Resolute",
	"America/Atikokan",
	"America/Rankin_Inlet",
	"America/Winnipeg",
	"America/Rainy_River",
	"America/Regina",
	"America/Swift_Current",
	"America/Edmonton",
	"America/Cambridge_Bay",
	"America/Yellowknife",
	"America/Inuvik",
	"America/Dawson_Creek",
	"America/Vancouver",
	"America/Whitehorse",
	"America/Dawson",
// Cayman Islands
	"America/Cayman",
// Chile
	"America/Santiago",
	"Pacific/Easter",
// Colombia
	"America/Bogota",
// Costa Rica
	"America/Costa_Rica",
// Cuba
	"America/Havana",
// Dominica
	"America/Dominica",
// Dominican Republic
	"America/Santo_Domingo",
// El Salvador
	"America/El_Salvador",
// Grenada
	"America/Grenada",
// Guam
	"GU", "Pacific/Guam",
// Guatemala
	"America/Guatemala",
// Guyana
	"America/Guyana",
// Haiti
	"HT", "America/Port-au-Prince",
// Honduras
	"HN", "America/Tegucigalpa",
// Jamaica
	"America/Jamaica",
// Japan
	"Asia/Tokyo",
// Marshall Islands
	"Pacific/Majuro",
	"Pacific/Kwajalein",
// Mexico
	"America/Mexico_City",
	"America/Cancun",
	"America/Merida",
	"America/Monterrey",
	"America/Matamoros",
	"America/Mazatlan",
	"America/Chihuahua",
	"America/Ojinaga",
	"America/Hermosillo",
	"America/Tijuana",
	"America/Santa_Isabel",
	"America/Bahia_Banderas",
// Micronesia
	"Pacific/Chuuk",
	"Pacific/Pohnpei",
	"Pacific/Kosrae",
// Midway Islands
	"Pacific/Midway",
// Montserrat
	"America/Montserrat",
// Netherlands Antilles
	"America/Curacao",
// Nicaragua
	"America/Managua",
// Northern Mariana Islands & Saipan
	"Pacific/Saipan",
// Palau
	"Pacific/Palau",
// Panama
	"America/Panama",
// Peru
	"America/Lima",
// Philippines
	"Asia/Manila",
// Puerto Rico
	"America/Puerto_Rico",
// Samoa
	"Pacific/Pago_Pago",
// South Korea
	"Asia/Seoul",
// Saint Kitts
	"America/St_Kitts",
// Saint Lucia
	"America/St_Lucia",
// Saint Vincent
	"America/St_Vincent",
// Surinam
	"America/Paramaribo",
// Taiwan
	"Asia/Taipei",
// Tonga
	"Pacific/Tongatapu",
// Trinidad & Tobago
	"America/Port_of_Spain",
// United States
	"America/New_York",
	"America/Detroit",
	"America/Kentucky/Louisville",
	"America/Kentucky/Monticello",
	"America/Indiana/Indianapolis",
	"America/Indiana/Vincennes",
	"America/Indiana/Winamac",
	"America/Indiana/Marengo",
	"America/Indiana/Petersburg",
	"America/Indiana/Vevay",
	"America/Chicago",
	"America/Indiana/Tell_City",
	"America/Indiana/Knox",
	"America/Menominee",
	"America/North_Dakota/Center",
	"America/North_Dakota/New_Salem",
	"America/Denver",
	"America/Boise",
	"America/Shiprock",
	"America/Phoenix",
	"America/Los_Angeles",
	"America/Anchorage",
	"America/Juneau",
	"America/Yakutat",
	"America/Nome",
	"America/Adak",
	"Pacific/Honolulu",
// Venezuela
	"America/Caracas",
// Virgin Islands
	"America/St_Thomas",
	"America/Tortola",
	0
};
