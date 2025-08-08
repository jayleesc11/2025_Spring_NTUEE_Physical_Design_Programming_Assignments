#include "config.h"

Config config;

void setConfig(const std::string &case_name, const std::string &alpha) {
    int case_id = 0;
    if (case_name.find("ami33") != std::string::npos) { case_id = 1; }
    else if (case_name.find("ami49") != std::string::npos) { case_id = 2; }
    else if (case_name.find("apte") != std::string::npos) { case_id = 3; }
    else if (case_name.find("hp") != std::string::npos) { case_id = 4; }
    else if (case_name.find("xerox") != std::string::npos) { case_id = 5; }

    int alpha_id = 0;
    if (alpha == "0.25") { alpha_id = 1; }
    else if (alpha == "0.5") { alpha_id = 2; }
    else if (alpha == "0.75") { alpha_id = 3; }

    switch (case_id) {
        case 1:
            switch(alpha_id) {
                case 1: // ami33_0.25
                    config = Config(0.98, 0.79, 2328, 575, 73, 23, 238);
                    break;
                case 2: // ami33_0.5
                    config = Config(0.99, 0.68, 2671, 311, 89, 18, 99);
                    break;
                case 3: // ami33_0.75
                    config = Config(0.99, 0.76, 2928, 688, 54, 6, 770);
                    break;
                default:
                    config = Config(0.98, 0.78, 2736, 933, 51, 17, 812);
            }
            break;
        case 2:
            switch(alpha_id) {
                case 1: // ami49_0.25
                    config = Config(0.93, 0.81, 2054, 467, 93, 18, 546);
                    break;
                case 2: // ami49_0.5
                    config = Config(0.87, 0.82, 1317, 310, 36, 15, 966);
                    break;
                case 3: // ami49_0.75
                    config = Config(0.94, 0.9, 1699, 790, 40, 6, 470);
                    break;
                default:
                    config = Config(0.98, 0.78, 2736, 933, 51, 17, 812);
            }
            break;
        case 3:
            switch(alpha_id) {
                case 1: // apte_0.25
                    config = Config(0.86, 0.78, 1922, 96, 5, 16, 106);
                    break;
                case 2: // apte_0.5
                    config = Config(0.92, 0.64, 1851, 589, 4, 17, 338);
                    break;
                case 3: // apte_0.75
                    config = Config(0.98, 0.87, 1349, 898, 100, 1, 520);
                    break;
                default:
                    config = Config(0.98, 0.78, 2736, 933, 51, 17, 812);
            }
            break;
        case 4:
            switch(alpha_id) {
                case 1: // hp_0.25
                    config = Config(0.8, 0.77, 1577, 755, 13, 14, 434);
                    break;
                case 2: // hp_0.5
                    config = Config(0.92, 0.61, 830, 768, 17, 24, 776);
                    break;
                case 3: // hp_0.75
                    config = Config(0.84, 0.81, 2009, 415, 11, 17, 83);
                    break;
                default:
                    config = Config(0.98, 0.78, 2736, 933, 51, 17, 812);
            }
            break;
        case 5:
            switch(alpha_id) {
                case 1: // xerox_0.25
                    config = Config(0.8, 0.85, 178, 753, 15, 24, 457);
                    break;
                case 2: // xerox_0.5
                    config = Config(0.9, 0.82, 2307, 938, 16, 9, 939);
                    break;
                case 3: // xerox_0.75
                    config = Config(0.87, 0.84, 1340, 252, 20, 7, 460);
                    break;
                default:
                    config = Config(0.98, 0.78, 2736, 933, 51, 17, 812);
            }
            break;
        default:
            config = Config(0.98, 0.78, 2736, 933, 51, 17, 812);
            break;
    }
}