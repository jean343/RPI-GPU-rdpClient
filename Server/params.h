#pragma once

#include <map>
#include <iostream>
#include <string>

using namespace std;

class Params {
public:

    Params(int argc, const char* argv[]) {
        // defaults
        monitor = -1;
        port = -1;
        
        map<string, string> params;
        for (int i = 1; i < argc; i++) {
            string key = argv[i];
            params[key] = argv[i + 1];
            i++;
        }

        typedef map<string, string>::iterator it_type;
        for (it_type iterator = params.begin(); iterator != params.end(); iterator++) {
            cout << iterator->first << " : " << iterator->second << endl;

            if (iterator->first.compare("monitor") == 0) {
                monitor = atoi(iterator->second.c_str());
            } else if (iterator->first.compare("port") == 0) {
                port = atoi(iterator->second.c_str());
            }
        }
    }

    int monitor;
    int port;
};