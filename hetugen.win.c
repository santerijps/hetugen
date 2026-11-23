/*
    Variables explained:
        c -> CENTURY
        y -> YEAR
        m -> MONTH
        d -> DAY
        s -> SEPARATOR
        h -> HASH

    Loop nesting:
        1. CENTURY
        2. SEPARATOR
        3. YEAR
        4. MONTH
        5. DAY
        6. INDIVIDUAL NUMBER

    Multithreading:
        Every line is exactly 12 bytes, so the file offset of every
        (century, separator, year) block is known up front. Each block is a
        work unit; worker threads claim units from a shared counter and write
        them directly into their own region of the memory mapped file.

        Usage: hetugen.win.exe [thread_count]
        (default: all logical processors)
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// index by: (1) is leap year (2) month
static const char DAYS_IN_MONTH_LOOKUP[2][16] = {
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

// index by: century (18 -> 1800s, 19th century)
static const char SEPARATORS_LOOKUP[32][8] = {
    [18] = { '+' },
    [19] = { '-', 'U', 'V', 'W', 'X', 'Y' },
    [20] = { 'A', 'B', 'C', 'D', 'E', 'F' },
};

// index by: int(ddmmyynnn) % 31
static const char HASH_LOOKUP[32] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'H', 'J', 'K', 'L',
    'M', 'N', 'P', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y',
};

// index by: day
static const char DAY_STRING_LOOKUP[32][4] = {
    "??", "01", "02", "03", "04",
    "05", "06", "07", "08", "09",
    "10", "11", "12", "13", "14",
    "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24",
    "25", "26", "27", "28", "29",
    "30", "31",
};

// index by: month
static const char MONTH_STRING_LOOKUP[16][4] = {
    "??", "01", "02", "03", "04",
    "05", "06", "07", "08", "09",
    "10", "11", "12",
};

// index by: year
static const char YEAR_STRING_LOOKUP[128][4] = {
    "00", "01", "02", "03", "04",
    "05", "06", "07", "08", "09",
    "10", "11", "12", "13", "14",
    "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24",
    "25", "26", "27", "28", "29",
    "30", "31", "32", "33", "34",
    "35", "36", "37", "38", "39",
    "40", "41", "42", "43", "44",
    "45", "46", "47", "48", "49",
    "50", "51", "52", "53", "54",
    "55", "56", "57", "58", "59",
    "60", "61", "62", "63", "64",
    "65", "66", "67", "68", "69",
    "70", "71", "72", "73", "74",
    "75", "76", "77", "78", "79",
    "80", "81", "82", "83", "84",
    "85", "86", "87", "88", "89",
    "90", "91", "92", "93", "94",
    "95", "96", "97", "98", "99",
};

// index by: n
static const char NNN_STRING_LOOKUP[1024][4] = {
    "???", "???", "002", "003", "004", "005", "006", "007", "008", "009",
    "010", "011", "012", "013", "014", "015", "016", "017", "018", "019",
    "020", "021", "022", "023", "024", "025", "026", "027", "028", "029",
    "030", "031", "032", "033", "034", "035", "036", "037", "038", "039",
    "040", "041", "042", "043", "044", "045", "046", "047", "048", "049",
    "050", "051", "052", "053", "054", "055", "056", "057", "058", "059",
    "060", "061", "062", "063", "064", "065", "066", "067", "068", "069",
    "070", "071", "072", "073", "074", "075", "076", "077", "078", "079",
    "080", "081", "082", "083", "084", "085", "086", "087", "088", "089",
    "090", "091", "092", "093", "094", "095", "096", "097", "098", "099",
    "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
    "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
    "120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
    "130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
    "140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
    "150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
    "160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
    "170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
    "180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
    "190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
    "200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
    "210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
    "220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
    "230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
    "240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
    "250", "251", "252", "253", "254", "255", "256", "257", "258", "259",
    "260", "261", "262", "263", "264", "265", "266", "267", "268", "269",
    "270", "271", "272", "273", "274", "275", "276", "277", "278", "279",
    "280", "281", "282", "283", "284", "285", "286", "287", "288", "289",
    "290", "291", "292", "293", "294", "295", "296", "297", "298", "299",
    "300", "301", "302", "303", "304", "305", "306", "307", "308", "309",
    "310", "311", "312", "313", "314", "315", "316", "317", "318", "319",
    "320", "321", "322", "323", "324", "325", "326", "327", "328", "329",
    "330", "331", "332", "333", "334", "335", "336", "337", "338", "339",
    "340", "341", "342", "343", "344", "345", "346", "347", "348", "349",
    "350", "351", "352", "353", "354", "355", "356", "357", "358", "359",
    "360", "361", "362", "363", "364", "365", "366", "367", "368", "369",
    "370", "371", "372", "373", "374", "375", "376", "377", "378", "379",
    "380", "381", "382", "383", "384", "385", "386", "387", "388", "389",
    "390", "391", "392", "393", "394", "395", "396", "397", "398", "399",
    "400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
    "410", "411", "412", "413", "414", "415", "416", "417", "418", "419",
    "420", "421", "422", "423", "424", "425", "426", "427", "428", "429",
    "430", "431", "432", "433", "434", "435", "436", "437", "438", "439",
    "440", "441", "442", "443", "444", "445", "446", "447", "448", "449",
    "450", "451", "452", "453", "454", "455", "456", "457", "458", "459",
    "460", "461", "462", "463", "464", "465", "466", "467", "468", "469",
    "470", "471", "472", "473", "474", "475", "476", "477", "478", "479",
    "480", "481", "482", "483", "484", "485", "486", "487", "488", "489",
    "490", "491", "492", "493", "494", "495", "496", "497", "498", "499",
    "500", "501", "502", "503", "504", "505", "506", "507", "508", "509",
    "510", "511", "512", "513", "514", "515", "516", "517", "518", "519",
    "520", "521", "522", "523", "524", "525", "526", "527", "528", "529",
    "530", "531", "532", "533", "534", "535", "536", "537", "538", "539",
    "540", "541", "542", "543", "544", "545", "546", "547", "548", "549",
    "550", "551", "552", "553", "554", "555", "556", "557", "558", "559",
    "560", "561", "562", "563", "564", "565", "566", "567", "568", "569",
    "570", "571", "572", "573", "574", "575", "576", "577", "578", "579",
    "580", "581", "582", "583", "584", "585", "586", "587", "588", "589",
    "590", "591", "592", "593", "594", "595", "596", "597", "598", "599",
    "600", "601", "602", "603", "604", "605", "606", "607", "608", "609",
    "610", "611", "612", "613", "614", "615", "616", "617", "618", "619",
    "620", "621", "622", "623", "624", "625", "626", "627", "628", "629",
    "630", "631", "632", "633", "634", "635", "636", "637", "638", "639",
    "640", "641", "642", "643", "644", "645", "646", "647", "648", "649",
    "650", "651", "652", "653", "654", "655", "656", "657", "658", "659",
    "660", "661", "662", "663", "664", "665", "666", "667", "668", "669",
    "670", "671", "672", "673", "674", "675", "676", "677", "678", "679",
    "680", "681", "682", "683", "684", "685", "686", "687", "688", "689",
    "690", "691", "692", "693", "694", "695", "696", "697", "698", "699",
    "700", "701", "702", "703", "704", "705", "706", "707", "708", "709",
    "710", "711", "712", "713", "714", "715", "716", "717", "718", "719",
    "720", "721", "722", "723", "724", "725", "726", "727", "728", "729",
    "730", "731", "732", "733", "734", "735", "736", "737", "738", "739",
    "740", "741", "742", "743", "744", "745", "746", "747", "748", "749",
    "750", "751", "752", "753", "754", "755", "756", "757", "758", "759",
    "760", "761", "762", "763", "764", "765", "766", "767", "768", "769",
    "770", "771", "772", "773", "774", "775", "776", "777", "778", "779",
    "780", "781", "782", "783", "784", "785", "786", "787", "788", "789",
    "790", "791", "792", "793", "794", "795", "796", "797", "798", "799",
    "800", "801", "802", "803", "804", "805", "806", "807", "808", "809",
    "810", "811", "812", "813", "814", "815", "816", "817", "818", "819",
    "820", "821", "822", "823", "824", "825", "826", "827", "828", "829",
    "830", "831", "832", "833", "834", "835", "836", "837", "838", "839",
    "840", "841", "842", "843", "844", "845", "846", "847", "848", "849",
    "850", "851", "852", "853", "854", "855", "856", "857", "858", "859",
    "860", "861", "862", "863", "864", "865", "866", "867", "868", "869",
    "870", "871", "872", "873", "874", "875", "876", "877", "878", "879",
    "880", "881", "882", "883", "884", "885", "886", "887", "888", "889",
    "890", "891", "892", "893", "894", "895", "896", "897", "898", "899"
};

// index by: year
static const char LEAP_YEAR_LOOKUP[4096] = {
    [1804] = 1, [1808] = 1, [1812] = 1, [1816] = 1,
    [1820] = 1, [1824] = 1, [1828] = 1, [1832] = 1,
    [1836] = 1, [1840] = 1, [1844] = 1, [1848] = 1,
    [1852] = 1, [1856] = 1, [1860] = 1, [1864] = 1,
    [1868] = 1, [1872] = 1, [1876] = 1, [1880] = 1,
    [1884] = 1, [1888] = 1, [1892] = 1, [1896] = 1,
    [1904] = 1, [1908] = 1, [1912] = 1, [1916] = 1,
    [1920] = 1, [1924] = 1, [1928] = 1, [1932] = 1,
    [1936] = 1, [1940] = 1, [1944] = 1, [1948] = 1,
    [1952] = 1, [1956] = 1, [1960] = 1, [1964] = 1,
    [1968] = 1, [1972] = 1, [1976] = 1, [1980] = 1,
    [1984] = 1, [1988] = 1, [1992] = 1, [1996] = 1,
    [2000] = 1, [2004] = 1, [2008] = 1, [2012] = 1,
    [2016] = 1, [2020] = 1, [2024] = 1, [2028] = 1,
    [2032] = 1, [2036] = 1, [2040] = 1, [2044] = 1,
    [2048] = 1, [2052] = 1, [2056] = 1, [2060] = 1,
    [2064] = 1, [2068] = 1, [2072] = 1, [2076] = 1,
    [2080] = 1, [2084] = 1, [2088] = 1, [2092] = 1,
    [2096] = 1, [2104] = 1, [2108] = 1, [2112] = 1,
};

// Every SSN line is exactly 12 bytes: DDMMYYsNNNH\n
#define LINE_LENGTH 12

// Individual numbers 002..899
#define INDIVIDUAL_NUMBERS_PER_DAY (899 - 2 + 1)

// One unit of work = one (century, separator, year) combination.
// 18xx: 1 separator, 19xx: 6 separators, 20xx: 6 separators -> 13 * 100 = 1300 units.
#define MAX_WORK_UNITS 2048

// Upper bound for worker threads (can be overridden on the command line).
#define MAX_THREADS 256

static const unsigned long long FILE_SIZE = 5116638768;

typedef struct {
    unsigned long long offset; // byte offset of the unit's first line in the output file
    short century;             // 18, 19 or 20
    short yy;                  // 0..99
    char separator;
} WorkUnit;

typedef struct {
    char *buffer;
    const WorkUnit *units;
    LONG unit_count;
    volatile LONG next_unit;
} SharedState;

typedef struct {
    SharedState *shared;
    unsigned long long count;
} ThreadContext;

// Writes all SSNs of a single (century, separator, year) unit into its
// own, non-overlapping region of the memory mapped file.
static unsigned long long generate_unit(char *buffer, const WorkUnit *unit) {
    char *bufptr = buffer + unit->offset;
    const int yy = unit->yy;
    const char s = unit->separator;
    const int is_leap_year = LEAP_YEAR_LOOKUP[unit->century * 100 + yy];
    const char *days_in_month_lookup = DAYS_IN_MONTH_LOOKUP[is_leap_year];
    unsigned long long count = 0;

    for (int m = 1; m <= 12; m += 1) {
        const int days_in_month = days_in_month_lookup[m];

        for (int d = 1; d <= days_in_month; d += 1) {
            for (int n = 2; n <= 899; n += 1) {
                const char h = HASH_LOOKUP[((d * 10000000) + (m * 100000) + (yy * 1000) + n) % 31];

                *bufptr++ = DAY_STRING_LOOKUP[d][0];
                *bufptr++ = DAY_STRING_LOOKUP[d][1];
                *bufptr++ = MONTH_STRING_LOOKUP[m][0];
                *bufptr++ = MONTH_STRING_LOOKUP[m][1];
                *bufptr++ = YEAR_STRING_LOOKUP[yy][0];
                *bufptr++ = YEAR_STRING_LOOKUP[yy][1];
                *bufptr++ = s;
                *bufptr++ = NNN_STRING_LOOKUP[n][0];
                *bufptr++ = NNN_STRING_LOOKUP[n][1];
                *bufptr++ = NNN_STRING_LOOKUP[n][2];
                *bufptr++ = h;
                *bufptr++ = '\n';

                count += 1;
            }
        }
    }

    return count;
}

// Workers pull units from a shared counter until none are left. Units are
// small (~4 MB each) so this balances load well, and threads stay close to
// each other in the file which keeps the OS write-back roughly sequential.
static DWORD WINAPI worker(LPVOID param) {
    ThreadContext *ctx = (ThreadContext*) param;
    SharedState *shared = ctx->shared;

    for (;;) {
        const LONG i = InterlockedIncrement(&shared->next_unit) - 1;
        if (i >= shared->unit_count) {
            break;
        }
        ctx->count += generate_unit(shared->buffer, &shared->units[i]);
    }

    return 0;
}

// Fills the work unit table in the same order as the original nested loops
// (century -> separator -> year), so the output file is byte-for-byte identical.
// Returns the total output size in bytes.
static unsigned long long build_work_units(WorkUnit *units, LONG *unit_count) {
    unsigned long long offset = 0;
    LONG count = 0;

    for (int c = 18; c <= 20; c += 1) {
        for (const char *separators = SEPARATORS_LOOKUP[c]; *separators; separators += 1) {
            for (int yy = 0; yy <= 99; yy += 1) {
                const int days_in_year = 365 + LEAP_YEAR_LOOKUP[c * 100 + yy];

                units[count].offset = offset;
                units[count].century = (short) c;
                units[count].yy = (short) yy;
                units[count].separator = *separators;
                count += 1;

                offset += (unsigned long long) days_in_year * INDIVIDUAL_NUMBERS_PER_DAY * LINE_LENGTH;
            }
        }
    }

    *unit_count = count;
    return offset;
}

static int default_thread_count(void) {
    DWORD n = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (n == 0) {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        n = info.dwNumberOfProcessors;
    }
    return n > 0 ? (int) n : 1;
}

int main(int argc, char **argv) {
    static WorkUnit units[MAX_WORK_UNITS];
    static HANDLE threads[MAX_THREADS];
    static ThreadContext contexts[MAX_THREADS];

    // Optional first argument: number of worker threads (default: all logical processors).
    int thread_count = argc > 1 ? atoi(argv[1]) : default_thread_count();
    if (thread_count < 1) thread_count = 1;
    if (thread_count > MAX_THREADS) thread_count = MAX_THREADS;

    LONG unit_count = 0;
    const unsigned long long total_size = build_work_units(units, &unit_count);

    if (total_size != FILE_SIZE) {
        fprintf(stderr, "Unexpected output size %llu (expected %llu)\n", total_size, FILE_SIZE);
        return 1;
    }

    LARGE_INTEGER freq, t_start, t_generated, t_end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t_start);

    HANDLE file_handle = CreateFileA(
        "out.txt",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFileA failed. Error: %lu\n", GetLastError());
        return 1;
    }

    HANDLE file_mapping_handle = CreateFileMappingA(
        file_handle,
        NULL,
        PAGE_READWRITE,
        (DWORD)(FILE_SIZE >> 32),
        (DWORD)(FILE_SIZE & 0xFFFFFFFF),
        NULL
    );

    if (file_mapping_handle == NULL) {
        fprintf(stderr, "CreateFileMappingA failed. Error: %lu\n", GetLastError());
        CloseHandle(file_handle);
        return 1;
    }

    char *buffer = (char*) MapViewOfFile(
        file_mapping_handle,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        FILE_SIZE
    );

    if (buffer == NULL) {
        fprintf(stderr, "MapViewOfFile failed. Error: %lu\n", GetLastError());
        CloseHandle(file_handle);
        CloseHandle(file_mapping_handle);
        return 1;
    }

    SharedState shared = {
        .buffer = buffer,
        .units = units,
        .unit_count = unit_count,
        .next_unit = 0,
    };

    int started = 0;
    for (int i = 0; i < thread_count; i += 1) {
        contexts[i].shared = &shared;
        contexts[i].count = 0;
        threads[i] = CreateThread(NULL, 0, worker, &contexts[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "CreateThread failed. Error: %lu\n", GetLastError());
            break;
        }
        started += 1;
    }

    // If no thread could be started, do the work on the main thread.
    unsigned long long count = 0;
    if (started == 0) {
        contexts[0].shared = &shared;
        contexts[0].count = 0;
        worker(&contexts[0]);
        count = contexts[0].count;
    }

    for (int i = 0; i < started; i += 1) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
        count += contexts[i].count;
    }

    QueryPerformanceCounter(&t_generated);

    // FlushViewOfFile(buffer, FILE_SIZE);
    UnmapViewOfFile(buffer);
    CloseHandle(file_mapping_handle);
    CloseHandle(file_handle);

    QueryPerformanceCounter(&t_end);

    fprintf(stderr, "Generated %llu SSNs using %d thread(s)!\n", count, started > 0 ? started : 1);
    fprintf(stderr, "Generate: %.3f s, unmap/close: %.3f s, total: %.3f s\n",
        (double)(t_generated.QuadPart - t_start.QuadPart) / freq.QuadPart,
        (double)(t_end.QuadPart - t_generated.QuadPart) / freq.QuadPart,
        (double)(t_end.QuadPart - t_start.QuadPart) / freq.QuadPart);

    return 0;
}
