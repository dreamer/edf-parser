#include <array>
#include <cstdio>
#include <deque>
#include <iostream>

struct edr_header
{
	unsigned record_no = 0;
	unsigned edr_no = 0;
	unsigned sattelite_flight_id = 0;
	char date[9] = { '\0' };
	char time[5] = { '\0' };
	char version[100] = { '\0' };
};

struct spacecraft_location
{
	double latitude = 0.;        // degrees, north
	double longitude = 0.;       // degrees, east
	double apex_latitude = 0.;   // degrees, north
	double apex_longitude = 0.;  // degrees, east
	double apex_local_time = 0.; // hours
	double altitude = 0.;        // km
};

enum class potential_source { CPU, SENPOT, };

enum class plasma_density_source { SM, DM, EP, }; 

struct edr
{
	edr_header header;
	spacecraft_location ephemeris[3];

	std::array<double, 15> satellite_potential; // volts
	potential_source potential_sensor;

	std::array<double, 60> plasma_density; // (one-second averages)(/cm³)
	plasma_density_source plasma_sensor;

	std::array<double, 60> horizontal_ion_drift; // m/s

	std::array<double, 60> vertical_ion_drift; // m/s
};

void discard_line(int & line)
{
	line++;
	char buff[120] = { '\0' }; // maximum line width is supposed to be 103
	                           // but that's not actually true
	scanf("%120[^\n]\n", buff);

	// std::cerr << "! discarded line: " << buff << std::endl;
}

edr_header parse_edr_header(int & line)
{
	int n = 0; // number of properly parsed items
	edr_header h;

	// blank line starting record
	line++;
	scanf("\n");

	// section header
	line++;
	n = scanf("RECORD, EDR OF RECORD, DMSP #, DATE, TIME - %120[^\n]\n",
	          h.version);

	line++;
	n = scanf("%u %u %u %s %s\n",
		  &h.record_no,
		  &h.edr_no,
		  &h.sattelite_flight_id,
		  h.date,
		  h.time);

	return h;
}

spacecraft_location parse_spacecraft_location(int & line)
{
	line++;
	spacecraft_location s;
	scanf("%lf %lf %lf %lf %lf %lf\n",
	      &s.latitude,
	      &s.longitude,
	      &s.apex_latitude,
	      &s.apex_longitude,
	      &s.apex_local_time,
	      &s.altitude);
	return s;
}

potential_source parse_potential_source(int & line)
{
	line++;
	int s = 0;
	scanf("%d\n", &s);
	switch (s) {
		case 1: return potential_source::CPU;
		case 2: return potential_source::SENPOT;
		default:
			std::cerr << "error on line " << line << ": "
			          << "unrecognized source (" << s << ")"
			          << std::endl;
			exit(1);
	}
}

plasma_density_source parse_plasma_density_source(int & line)
{
	line++;
	int s = 0;
	scanf("%d\n", &s);
	switch (s) {
		case 1: return plasma_density_source::SM;
		case 2: return plasma_density_source::DM;
		case 3: return plasma_density_source::EP;
		default:
			std::cerr << "error on line " << line << ": "
			          << "unrecognized source (" << s << ")"
			          << std::endl;
			exit(1);
	}
}

edr parse_edr(int & line)
{
	const auto end_line = line + 114;

	edr r;
	r.header = parse_edr_header(line);

	discard_line(line); // EPHEMERIS
	r.ephemeris[0] = parse_spacecraft_location(line);
	r.ephemeris[1] = parse_spacecraft_location(line);
	r.ephemeris[2] = parse_spacecraft_location(line);

	discard_line(line); // SATTELITE POTENTIAL (…)
	line++; // counting all 15 values as if they were in single line
	for (auto & x : r.satellite_potential)
		scanf("%lf", &x);
	r.potential_sensor = parse_potential_source(line);

	discard_line(line); // PRIMARY PLASME DENSITY (…)
	line += 10;
	for (auto & p : r.plasma_density)
		scanf("%lf", &p);
	r.plasma_sensor = parse_plasma_density_source(line);

	discard_line(line); // HORIZONTAL ION DRIFT VELOCS
	line += 10;
	for (auto & p : r.horizontal_ion_drift)
		scanf("%lf", &p);
	scanf("\n");

	discard_line(line); // VERTICAL ION DRIFT VELOCS
	line += 10;
	for (auto & p : r.vertical_ion_drift)
		scanf("%lf", &p);
	scanf("\n");

	// TODO continue here:

	// FIXME remove when whole parser is implemented
	// discard rest of edr
	for (auto i = line; i < end_line; ++i)
		discard_line(line);

	return r;
}

int main()
{
	int line_counter = 0;

	std::deque<edr> all_records = {};

	while (!feof(stdin))
		all_records.push_back(parse_edr(line_counter));

	std::cout << "lines parsed: " << line_counter << std::endl;
	std::cout << "records read: " << all_records.size() << std::endl;

	const auto example = all_records[42]; // line 4790
	std::cout << "example record: "     << example.header.record_no << std::endl;
	std::cout << "example record edr: " << example.header.edr_no << std::endl;
	std::cout << "example values for vertical ion drift: " << std::endl;
	for (const auto x : example.vertical_ion_drift)
		std::cout << x << std::endl;

	return 0;
}
