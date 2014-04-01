//
// Copyright 2014 Jared H. Hudson
//
// This file is part of Websysstat.
//
// Websysstat is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
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

using namespace std;
using namespace httpserver;

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
    string buffer;
    buffer = "{\n";
    buffer += "\t\"min1\": {\n";
    buffer += "\t\t\"label\": \"1 min load average\",\n";
    buffer += "\t\t\"data\": [[";
    time_t t = time(NULL);
    datestr << (t*1000);
    buffer += datestr.str();
    buffer += ", ";
    
    ifstream file("/proc/loadavg",ios_base::in);
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
      cout << "loadavg: " << min1 << " " << min5 << " " << min15 << " " 
	   << runqueue << " " << last_pid << endl;
    }
    datastr << min1;
    buffer += datastr.str();
    buffer += "]]},\n";
    
    buffer += "\t\"min5\": {\n";
    buffer += "\t\t\"label\": \"5 min load average\",\n";
    buffer += "\t\t\"data\": [[";
    buffer += datestr.str();
    buffer += ", ";
    datastr.str("");
    datastr << min5;
    buffer += datastr.str();
    buffer += "]]}\n";
    buffer += "}\n";

    cout << buffer << endl;
    *res = new http_string_response(buffer, 200, "text/html");    
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
	cout << device << " " << time_io << endl;
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
    cout << buffer << endl;
    *res = new http_string_response(buffer, 200, "text/html");
  } else {
    string cwd(get_current_dir_name());
    path = cwd + path;
    cout << "Opening " << path << endl;
    ifstream file(path.c_str(),ios_base::in|ios_base::ate);
    if (file.is_open()) {
      cout << "opened" << endl;
      file_size = file.tellg();
      cout << "size: " << file_size << endl;
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

int main(int argc, char **argv)
{
  int port = 8080;

  if (argc > 1) {
    port = atoi(argv[1]);
  }
  chdir("htdocs");
  webserver ws = create_webserver(port).max_threads(5);
  main_resource mr;

  ws.register_resource("/", &mr, true);
  ws.start(true);
  return 0;
}
  
