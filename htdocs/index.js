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
   
$(function() {
    var startTime = new Date().getTime();
    var options = {
	lines: {
	    show: true
	},
	xaxis: {
	    mode: "time",
	    timeformat: "%H:%M:%S",
	},
	yaxis: {
	    min: 0,
	    zoomRange: [0.0, 10],
	    panRange: [0, 10]
	    },
	zoom: {
	    interactive: true
	    },
	pan: {
	    interactive: true
	    },
	grid: {
	    margin: { 
		left: 50,
		right: 50
	    },
	}
    };

    var data = [];
    var data2 = [];
    var label = [];
    var label2 = "";
    var debug = "";
    var disable_min = false;
    var update_count = 0;
    var x_dps = 60; // x-axis data points
    var plot = $.plot("#graph", [ ], options);
    
    $("#footer").append("- Flot " + $.plot.version);
    
    function update() {
	startTime = new Date().getTime();

	update_count++;
	if (data[0].length > x_dps && disable_min == false) {
	    var largest_y = 0.0, i;

	    for(var j=0; j < data.length; j++) {
		for(i=data[j].length-x_dps-1; i < data[j].length; i++) {
		    if (i < 0) {
			i = 0;
		    }

		    if (typeof data[j][i] === 'undefined') {
			console.log("j="+j+" i="+i+" data.length="+data.length);
		    }

		    if (data[j][i][1] > largest_y) {
			largest_y = data[j][i][1];
		    }
		}
	    }

	    plot.getOptions().yaxes[0].max = largest_y + largest_y*0.20;
	    plot.getOptions().xaxes[0].min = data[0][data[0].length-1][0]-(x_dps*1000);;
	    plot.getOptions().xaxes[0].max = data[0][data[0].length-1][0];

	    console.log("yaxes.max="+plot.getOptions().yaxes[0].max);
	    console.log("xaxes.min="+plot.getOptions().xaxes[0].min);
	    console.log("xaxes.max="+plot.getOptions().xaxes[0].max);

	} else if (data[0].length == 1) {
	    plot.getOptions().xaxes[0].max = data[0][0][0] + (x_dps*1000);
	}

	var datapoints = [[]];
	for(i=0; i < data.length; i++) {
	    datapoints[i] = {};
	    datapoints[i].label = label[i];
	    datapoints[i].data = data[i];
	}

	plot.setData(datapoints);
	plot.setupGrid();
	plot.draw();

	$("#header").text("js execution time: " + (new Date().getTime() - startTime) + "ms" );	
	$("#update_count").text("data points: " + data[0].length);	
    }
    
    function fetchData() {
	function onDataReceived(series) {
	    var i = 0;
	    for (prop in series) {
		if (typeof data[i] === 'undefined') {
		    data[i] = [];
		}
		label[i] = series[prop].label;
		data[i].push([series[prop].data[0][0],series[prop].data[0][1]]);
		i++;
	    }
	    update()
	}
	
	$.ajax({
	    url: "/loadavg",
	    type: "GET",
	    dataType: "json",
	    success: onDataReceived
	});
	
	setTimeout(fetchData, 1000);
    }

    $("#graph").bind("plotpan", function (event, plot) {
	disable_min = true;
    });

    $("#graph").bind("plotzoom", function (event, plot) {
	disable_min = true;	
    });

    $("#reset_view").click(function (e) {
	e.preventDefault();
	disable_min = false;
	update();
    });

    function onDataReceived(series) {
	var i;
	
	for (i=0; i < 4; i++) {
	    if (typeof data[i] === 'undefined') {
		data[i] = [];
	    }
	}
	i = 0;
	
	for (prop in series) {
	    for (entry in series[prop].hosts[0].statistics) {
		var date = series[prop].hosts[0].statistics[entry].timestamp.date;
		var time = series[prop].hosts[0].statistics[entry].timestamp.time;
		var timestamp = Date.parse(date + "T" + time + "-0000");
		var load1 = series[prop].hosts[0].statistics[entry].queue['ldavg-1'];
		var load5 = series[prop].hosts[0].statistics[entry].queue['ldavg-5'];
		var load15 = series[prop].hosts[0].statistics[entry].queue['ldavg-15'];
		
		i = 0;
		label[i] = "ldavg-1";
		data[i].push([timestamp, load1]);
		i++;
		
		label[i] = "ldavg-5";
		data[i].push([timestamp, load5]);
		i++;
		
		label[i] = "ldavg-15";
		data[i].push([timestamp, load15]);
		i++;
	    }
	}
	
	update()
    }
  
    $.ajax({
	url: "/sysstat",
	type: "GET",
	dataType: "json",
	success: onDataReceived
    }); 
    
    setTimeout(fetchData, 1000);

    // Time execution
    $("#header").text("js execution time: " + (new Date().getTime() - startTime) + "ms" );
});

