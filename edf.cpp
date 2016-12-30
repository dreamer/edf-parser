#include <array>
#include <cstdio>
#include <deque>
#include <iostream>
#include <chrono>

using seconds = std::chrono::seconds;

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

enum class potential_source
{
	CPU    = 1,
	SENPOT = 2,
};

enum class plasma_density_source
{
	SM = 1,
	DM = 2,
	EP = 3,
};

enum class ckl_qualifier
{
	NO_ANALYSIS_ATTEMPT     = 0,
	NO_ANALYSIS_NO_DATA     = 1,
	NO_ANALYSIS_RMS_TOO_LOW = 2,
	ANALYSIS_256_POINTS     = 3,
	ANALYSIS_512_POINTS     = 4,
};

enum class ckl_source
{
	SM_DENSITY_DATA        = 1,
	SM_DENSITY_FILTER_DATA = 2,
	EP_DC_DENSITY_DATA     = 3,
};

enum class ep_source
{
	GROUND = 1, // Ground processing analysis
	CPU    = 2, // On-board microprocessor analysis
};

using rpa_source = ep_source;

struct ckl_analysis
{
	double rms = 0.;
	double t1  = 0.;
	double p1  = 0.;
	double ckl = 0.;
	std::array<double, 15> power_density_spectrum;
	ckl_qualifier qualifier;
};

struct ckl_analyses
{
	std::array<ckl_analysis, 6> analyses;
	ckl_source data_used;
};

struct ep_sweep_analysis
{
	seconds sweep_center_time; // UT
	double electron_density; // el/cm³
	double electron_temperature; // K
	double satellite_potential; // volts
	int qualifier; // TODO format description not clear - investigate
	double surrogate; // EP photo-electron surrogate value
};

struct ep_sweep_analyses
{
	std::array<ep_sweep_analysis, 15> sets;
	ep_source source;
};

struct rpa_sweep_analysis
{
	seconds sweep_center_time; // UT
	double op_density;         // O+ density (ion/cm³)
	double hp_hep_density;     // Total (H+ + He+) density (ion/cm³)
	int light_ion_flag;        // Light ion flag (integer)
                                   // 0  - No light ion
                                   // 1  - Light ion is H+
                                   // 2  - Light ion is He+
                                   // 3+ - = 3 + 10000 x (H+ fraction)
	double ion_temperature;    // K
	double ion_drift;          // Ram ion drift velocity (m/s)
	int qualifier;             // FIXME replace with state
			           // 0 - Analysis terminated unsuccessfully
                                   // 1 - Successful analysis
	double ion_density;        // RPA-derived total ion density

	// Note some records may only have valid values for field 1 and 8,
	// these will have a value of 0 in field 7.
};

struct rpa_sweep_analyses
{
	std::array<rpa_sweep_analysis, 15> sets;
	rpa_source source;
};

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

	ckl_analyses ckl;

	ep_sweep_analyses ep;

	rpa_sweep_analyses rpa;
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

template<typename Source>
Source parse_source(int & line)
{
	line++;
	int s = -1;
	scanf("%d\n", &s);
	return Source(s);
}

ckl_analysis parse_ckl_analysis(int & line)
{
	ckl_analysis a;

	line++;
	scanf("%lf %lf %lf %lf\n",
	      &a.rms,
	      &a.t1,
	      &a.p1,
	      &a.ckl);

	line++;
	for (auto & x : a.power_density_spectrum)
		scanf("%lf", &x);

	a.qualifier = parse_source<ckl_qualifier>(line);

	return a;
}

ckl_analyses parse_ckl_analyses(int & line)
{
	discard_line(line); // CKL ANALYSES, THEN SOURCE

	ckl_analyses a;

	for (auto & x : a.analyses)
		x = parse_ckl_analysis(line);

	a.data_used = parse_source<ckl_source>(line);

	return a;
}

ep_sweep_analysis parse_ep_sweep_analysis(int & line)
{
	line++;
	ep_sweep_analysis a;

	int time = 0;
	scanf("%d %lf %lf %lf %d %lf\n",
	      &time,
	      &a.electron_density,
	      &a.electron_temperature,
	      &a.satellite_potential,
	      &a.qualifier,
	      &a.surrogate);
	a.sweep_center_time = seconds(time);

	return a;
}

// FIXME this should sometimes parse sections:
//       EP AVERAGE DENSITIES
//	 and some more, but it's missing from example data
ep_sweep_analyses parse_ep_sweep_analyses(int & line)
{
	discard_line(line); // EP SWEEP ANALYSES SETS

	ep_sweep_analyses e;

	for (auto & s : e.sets)
		s = parse_ep_sweep_analysis(line);

	discard_line(line); // EP ANALYSES SOURCE

	e.source = parse_source<ep_source>(line);

	return e;
}

rpa_sweep_analysis parse_rpa_sweep_analysis(int & line)
{
	line++;
	rpa_sweep_analysis a;

	int time = 0;
	int qualifier = 0;
	scanf("%d %lf %lf %d %lf %lf %d %lf\n",
	      &time,
	      &a.op_density,
	      &a.hp_hep_density,
	      &a.light_ion_flag,
	      &a.ion_temperature,
	      &a.ion_drift,
	      &qualifier,
	      &a.ion_density);

	a.sweep_center_time = seconds(time);
	a.qualifier = qualifier;

	return a;
}

rpa_sweep_analyses parse_rpa_sweep_analyses(int & line)
{
	discard_line(line); // RPA SWEEP ANALYSES SETS, THEN SOURCE

	rpa_sweep_analyses r;

	for (auto & s : r.sets)
		s = parse_rpa_sweep_analysis(line);

	r.source = parse_source<rpa_source>(line);

	return r;
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
	r.potential_sensor = parse_source<potential_source>(line);

	discard_line(line); // PRIMARY PLASME DENSITY (…)
	line += 10;
	for (auto & p : r.plasma_density)
		scanf("%lf", &p);
	r.plasma_sensor = parse_source<plasma_density_source>(line);

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

	r.ckl = parse_ckl_analyses(line);

	r.ep = parse_ep_sweep_analyses(line);

	r.rpa = parse_rpa_sweep_analyses(line);

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

	const auto example = all_records[0]; // line 4790
	std::cout << "example record: "     << example.header.record_no << std::endl;
	std::cout << "example record edr: " << example.header.edr_no << std::endl;
	/*
	std::cout << "example values for vertical ion drift: " << std::endl;
	for (const auto x : example.vertical_ion_drift)
		std::cout << x << std::endl;
	*/
	std::cout << example.ep.sets[14].surrogate << std::endl;

	return 0;
}
