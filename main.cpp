//
// Copyright 2014 Jared H. Hudson
//
// This file is part of Websysstat.
//
// Websysstat is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#include <httpserver.hpp>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sstream>
#include <json/json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace httpserver;

bool daemonize = false;

class main_resource : public http_resource<main_resource> {
public:
  main_resource() : last(0) { }
  void render(const http_request &, http_response **);
private:
  int last;
};

void main_resource::render(const http_request &req, http_response **res)
{
  streampos file_size;
  int filemem_size;
  char *buffer = NULL;
  stringstream datestr, datastr;
  string path = req.get_path();

  if (path.empty() || path == "/") {
    path = string("/index.html");
  }

  if (path == "/loadavg") {
    json_object *min1_jo = json_object_new_object();
    json_object *min5_jo = json_object_new_object();
    json_object *min15_jo = json_object_new_object();
    json_object *data = json_object_new_array();
    json_object *data_holder = json_object_new_array();
    json_object *load_jo = json_object_new_object();

    time_t t = time(NULL);

    ifstream file("/proc/loadavg", ios_base::in);
    float min1, min5, min15;
    string runqueue;
    pid_t last_pid;

    if (file.is_open()) {
      string s;
      while (!file.eof()) {
	file >> min1 >> min5 >> min15;
	file >> runqueue;
	file >> last_pid;
      }
      file.close();
      if (!daemonize) {
	cout << "loadavg: " << min1 << " " << min5 << " " << min15 << " " 
	     << runqueue << " " << last_pid << endl;
      }
    }

    json_object_object_add(min1_jo, "label", json_object_new_string("1 min load average"));
    json_object_array_add(data, json_object_new_int(t*1000));   
    json_object_array_add(data, json_object_new_double(min1));
    json_object_array_add(data_holder, data);
    json_object_object_add(min1_jo, "data", data_holder);

    data = json_object_new_array();
    data_holder = json_object_new_array();
    json_object_object_add(min5_jo, "label", json_object_new_string("5 min load average"));
    json_object_array_add(data, json_object_new_int(t*1000));
    json_object_array_add(data, json_object_new_double(min5));
    json_object_array_add(data_holder, data);
    json_object_object_add(min5_jo, "data", data_holder);

    data = json_object_new_array();
    data_holder = json_object_new_array();
    json_object_object_add(min15_jo, "label", json_object_new_string("15 min load average"));
    json_object_array_add(data, json_object_new_int(t*1000));
    json_object_array_add(data, json_object_new_double(min15));
    json_object_array_add(data_holder, data);
    json_object_object_add(min15_jo, "data", data_holder);

    json_object_object_add(load_jo, "min1", min1_jo);
    json_object_object_add(load_jo, "min5", min5_jo);
    json_object_object_add(load_jo, "min15", min15_jo);

    if (!daemonize) {
      cout << json_object_to_json_string(load_jo) << endl;
    }
    *res = new http_string_response(json_object_to_json_string(load_jo), 200, "text/html");

    json_object_put(load_jo);
  } else if (path == "/diskstats") {
    string buffer;
    buffer = "{\n";
    buffer += "\"label\": \"test\",\n";
    buffer += "\"data\": [[";
    time_t t = time(NULL);
    datestr << (t*1000);
    buffer += datestr.str();
    buffer += ", ";
    
    ifstream file("/proc/diskstats",ios_base::in);
    int major, minor;
    string device;
    int reads, reads_merged, sectors_read, time_reads;
    int writes, writes_merged, sectors_written, time_writes;
    int io_progress, time_io, weighted_time_io;

    if (file.is_open()) {
      string s;
      while (!file.eof()) {
	file >> major >> minor;
	file >> device;

	if (device != "sda3") {
	  getline(file, s);
	  continue;
	}

	file >> reads >> reads_merged >> sectors_read >> time_reads;
	file >> writes >> writes_merged >> sectors_written >> time_writes;
	file >> io_progress >> time_io >> weighted_time_io;
	if (!daemonize) {
	  cout << device << " " << time_io << endl;
	}
      }
      file.close();
    }
    if (last == 0) {
      last = reads;
    }
    datastr << reads-last;
    last = reads;

    buffer += datastr.str();
    buffer += "]]\n";
    buffer += "}\n";
    if (!daemonize) {
      cout << buffer << endl;
    }
    *res = new http_string_response(buffer, 200, "text/html");
  } else {
    string cwd(get_current_dir_name());
    path = cwd + path;
    if (!daemonize) {
      cout << "Opening " << path << endl;
    }
    ifstream file(path.c_str(),ios_base::in|ios_base::ate);
    if (file.is_open()) {
      if (!daemonize) {
	cout << "opened" << endl;
      }
      file_size = file.tellg();
      if (!daemonize) {
	cout << "size: " << file_size << endl;
      }
      filemem_size = file_size;
      buffer = new char [filemem_size];
      file.seekg(0, ios::beg);
      file.read(buffer, file_size);
      buffer[filemem_size-1] = 0;
      file.close();
    }

    if (buffer == NULL) {
      *res = new http_string_response("No file found.", 404, "text/plain");
    } else {
      if (path.compare(path.length()-4,4,".css") == 0) {
	*res = new http_string_response(buffer, 200, "text/css");
      } else {
	*res = new http_string_response(buffer, 200, "text/html");
      }
    }
  }
}

int main(int argc, char * argv[])
{
  int32_t opt;
  while ((opt = getopt(argc, argv, "d?h")) != -1) {
    switch (opt) {
    case 'd':
      daemonize = true;
      break;
    case 'h':
    case '?':
    default:
      cerr << "Usage: " << argv[0] << " [-d]\n";
      cerr << "-d\t\tdaemonize\n" << endl;
      exit(EXIT_FAILURE);
    }
  }

  if (daemonize) {
    pid_t pid, sid;
    
    if ((pid = fork()) < 0) {
      exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
      exit(EXIT_SUCCESS);
    }
    
    umask(0);

    if ((sid = setsid()) < 0) {
      perror("setsid");
      exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  int port = 8080;

  chdir("htdocs");
  webserver ws = create_webserver(port).max_threads(5);
  if (!ws) {
    cerr << "error" << endl;
  }

  main_resource mr;
  ws.register_resource("/", &mr, true);
  ws.start(true);
  
  return 0;
}
  
