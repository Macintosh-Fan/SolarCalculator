#include "SolarCalculator.h"

//
// Intermediate calculations
// Time T is measured in Julian Centuries (36525 ephemeris days from the epoch J2000.0)
//

double between0And1(double n)
{
    return n + (n < 0) - (n > 1);
}

double interpolateCoordinates(double n, double y1, double y2, double y3)
{
    if ((y1 < 0) != (y2 < 0) && fabs(y1) > 25)      // if right ascension is discontinuous
        y1 += ((y1 < 0) - (y2 < 0)) * 360;          // add or subtract 360 degrees
    else if ((y3 < 0) != (y2 < 0) && fabs(y3) > 25)
        y3 += ((y3 < 0) - (y2 < 0)) * 360;
    double a = y2 - y1;
    double b = y3 - y2;
    double c = b - a;
    return y2 + n * (a + b + n * c) / 2;
}

double fractionalDay(int hour, int minute, int second)
{
    return (hour + minute / 60.0 + second / 3600.0) / 24;
}

double calcJulianDay(int year, int month, int day)
{
    if (month <= 2)
    {
        year -= 1;
        month += 12;
    }
    double A = floor(year / 100.0);
    double B = 2 - A + floor(A / 4);
    return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

double calcJulianCent(double JD)
{
    return (JD - 2451545) / 36525;
}

double calcJulianCentSplit(double JD, double m)
{
    double T = calcJulianCent(JD);
    return T + m / 36525;
}

double calcGeomMeanLongSun(double T)
{
    return fmod(280.46646 + T * (36000.76983 + 0.0003032 * T), 360);  // in degrees
}

double calcGeomMeanAnomalySun(double T)
{
    return 357.52911 + T * (35999.05029 - 0.0001537 * T);   // in degrees
}

double calcEccentricityEarthOrbit(double T)
{
    return 0.016708634 - T * (0.000042037 + 0.0000001267 * T);  // no units
}

double calcSunEqOfCenter(double T)
{
    double M = radians(calcGeomMeanAnomalySun(T));
    return sin(M) * (1.914602 - T * (0.004817 + 0.000014 * T)) + sin(2 * M) * (0.019993 - 0.000101 * T) +
           sin(3 * M) * 0.000289;   // in degrees
}

double calcSunTrueLong(double T)
{
    return calcSunEqOfCenter(T) + calcGeomMeanLongSun(T);   // in degrees
}

double calcSunTrueAnomaly(double T)
{
    return calcGeomMeanAnomalySun(T) + calcSunEqOfCenter(T);    // in degrees
}

double calcSunRadVector(double T) {
    double v = radians(calcSunTrueAnomaly(T));
    double e = calcEccentricityEarthOrbit(T);
    return (1.000001018 * (1 - e * e)) / (1 + e * cos(v));  // in AUs
}

double calcSunApparentLong(double T)
{
    double O = calcSunTrueLong(T);
    double omega = radians(125.04 - 1934.136 * T);
    return O - 0.00569 - 0.00478 * sin(omega);  // in degrees
}

double calcMeanObliquityOfEcliptic(double T)
{
    return 23 + (26 + (21.448 - T * (46.815 + T * (0.00059 - 0.001813 * T))) / 60) / 60;    // in degrees
}

// See Astronomical Algorithms 2nd edition errata
double calcNutationLongitude(double T)
{
    double omega = radians(125.04452 - 1934.136261 * T);
    double L1 = radians(280.4665 + 36000.7698 * T);
    double L2 = radians(218.3165 + 481267.8813 * T);
    return - 17.2 * sin(omega) - 1.32 * sin(2 * L1) - 0.23 * sin(2 * L2) + 0.21 * sin(2 * omega);   // in arcseconds
}

double calcNutationObliquity(double T)
{
    double omega = radians(125.04452 - 1934.136261 * T);
    double L1 = radians(280.4665 + 36000.7698 * T);
    double L2 = radians(218.3165 + 481267.8813 * T);
    return 9.20 * cos(omega) + 0.57 * cos(2 * L1) + 0.10 * cos(2 * L2) - 0.09 * cos(omega); // in arcseconds
}

double calcObliquityCorrection1(double T)
{
    double epsilon0 = calcMeanObliquityOfEcliptic(T);
    double omega = radians(125.04 - 1934.136 * T);
    return epsilon0 + 0.00256 * cos(omega); // in degrees
}

double calcObliquityCorrection2(double T)
{
    double epsilon0 = calcMeanObliquityOfEcliptic(T);
    double delta_epsilon = calcNutationObliquity(T);
    return epsilon0 + delta_epsilon / 3600; // in degrees
}

double calcSunRtAscension(double T)
{
    double epsilon = radians(calcObliquityCorrection1(T));
    double lambda = radians(calcSunApparentLong(T));
    return degrees(atan2(cos(epsilon) * sin(lambda), cos(lambda))); // in degrees
}

double calcSunDeclination(double T)
{
    double epsilon = radians(calcObliquityCorrection1(T));
    double lambda1 = radians(calcSunApparentLong(T));
    return degrees(asin(sin(epsilon) * sin(lambda1)));  // in degrees
}

// We neglect the very small variation of delta_psi during time ΔT
double calcNutationRtAscension(double T)
{
    double delta_psi = calcNutationLongitude(T);
    double epsilon = radians(calcObliquityCorrection2(T));
    return delta_psi * cos(epsilon) / 3600; // in degrees
}

// Valid only at 0h UT, Greenwich (JD ending in .5)
double calcMeanSiderealTime(double JD)
{
    double T = calcJulianCent(JD);
    return fmod(100.46061837 + T * (36000.770053608 + T * (0.000387933 - T / 38710000)), 360); // in degrees
}

double calcApparentSiderealTime(double JD)
{
    double T = calcJulianCent(JD);
    return calcMeanSiderealTime(JD) + calcNutationRtAscension(T);   // in degrees
}

// We neglect the small variation of delta_psi during time m
double calcSiderealTimeInstant(double GAST, double m)
{
    return fmod(GAST + 360.985647 * m, 360);  // in degrees
}

/*
// Alternate implementation
//
// Valid only at 0h UT, Greenwich (JD ending in .5)
double calcMeanSiderealTime(double JD)
{
    double T = calcJulianCent(JD);
    return fmod(100.46061837 + T * (36000.770053608 + T * (0.000387933 - T / 38710000)), 360); // in degrees
}

double calcMeanSiderealTimeInstant(double JD, double m)
{
    double theta0 = calcMeanSiderealTime(JD);
    return fmod(theta0 + 360.985647 * m, 360);    // in degrees
}

double calcApparentSiderealTime(double JD)
{
    double T = calcJulianCent(JD);
    return calcMeanSiderealTime(JD) + calcNutationRtAscension(T);   // in degrees
}

double calcApparentSiderealTimeInstant(double JD, double m)
{
    double T = calcJulianCentSplit(JD, m);
    return calcMeanSiderealTimeInstant(JD, m) + calcNutationRtAscension(T); // in degrees
}
*/

double calcSolarElevation(double ha, double decl, double lat)
{
    return degrees(asin(sin(radians(lat)) * sin(radians(decl)) +
                   cos(radians(lat)) * cos(radians(decl)) * cos(radians(ha))));    // in degrees
}

double calcSolarAzimuth(double ha, double decl, double lat)
{
    return degrees(M_PI + atan2(sin(radians(ha)), cos(radians(ha)) * sin(radians(lat)) -
                   tan(radians(decl)) * cos(radians(lat))));   // in degrees
}

// Model of approximate atmospheric refraction
// https://www.esrl.noaa.gov/gmd/grad/solcalc/calcdetails.html
//
double calcRefractionCorr(double elev)
{
    if (elev > 85.0)
        return 0.0;
    else
    {
        double te = tan(radians(elev));
        if (elev > 5.0)
            return (58.1 / te - 0.07 / (te * te * te) + 0.000086 / (te * te * te * te * te)) / 3600;
        else if (elev > -0.575)
            return (1735.0 + elev * (-518.2 + elev * (103.4 + elev * (-12.79 + 0.711 * elev)))) / 3600;
        else
            return (-20.774 / te) / 3600;   // in degrees
    }
}

// The Equation of (Ephemeris) Time, in minutes of time
//
// By W.M. Smart
// Textbook on Spherical Astronomy (1971)
//
double equationOfTime1(double T)
{
    double e = calcEccentricityEarthOrbit(T);
    double L = radians(calcGeomMeanLongSun(T));
    double M = radians(calcGeomMeanAnomalySun(T));
    double epsilon = radians(calcObliquityCorrection2(T));
    double y = tan(epsilon / 2) * tan(epsilon / 2);
    return 4 * degrees(y * sin(2 * L) - 2 * e * sin(M) + 4 * e * y * sin(M) * cos(2 * L) - 0.5 * y * y * sin(4 * L) -
                       1.25 * e * e * sin(2 * M)); // in minutes of time
}

// By D.W. Hughes
// http://adsabs.harvard.edu/full/1989MNRAS.238.1529H
//
double equationOfTime2(double T)
{
    double e = calcEccentricityEarthOrbit(T);
    double L = radians(calcGeomMeanLongSun(T));
    double M = radians(calcGeomMeanAnomalySun(T));
    double epsilon = radians(calcObliquityCorrection2(T));
    double y = tan(epsilon / 2) * tan(epsilon / 2);
    return 4 * degrees(0.00000447 * T + 0.00000149 * T * T - 2 * e * sin(M) - 1.25 * e * e * sin(2 * M) +
                       y * sin(2 * L) - 0.5 * y * y * sin (4 * L) + 4 * e * y * sin(M) * cos(2 * L) +
                       2.5 * e * e * y * sin(2 * M) * cos(2 * L) - 4 * e * y * y * sin(M) * cos(4 * L) -
                       8 * e * e * y * sin(M) * sin(M) * sin(2 * L));  // in minutes of time
}

// As defined by Jean Meeus
// Astronomical Algorithms (1991)
//
double equationOfTime3(double T)
{
    double alpha = calcSunRtAscension(T);
    double delta_psi = calcNutationLongitude(T);
    double epsilon = radians(calcObliquityCorrection2(T));
    double t = T / 10;
    double L0 = fmod(280.4664567 + 360007.6982779 * t + 0.03032028 * (t * t) + (t * t * t) / 49931 -
                     (t * t * t * t) / 15300 - (t * t * t * t * t) / 2000000, 360);
    return 4 * (L0 - 0.0057183 - alpha + delta_psi * cos(epsilon) / 3600);  // in minutes of time
}

// Polynomial Expressions for Delta T (ΔT), in seconds of time
//
// By Fred Espenak, valid from year -1999 to +3000
// http://eclipsewise.com/help/deltatpoly2014.html
//
double calcDeltaT(double year, double month)
{
    double y = year + (month - 0.5) / 12;
    if (2015 < y)
    {
        double t = y - 2015;
        return 67.62 + 0.3645 * t + 0.0039755 * pow(t, 2);
    }
    if (2005 <= y && y <= 2015)
    {
        double t = y - 2005;
        return 64.69 + 0.2930 * t;
    }
    else if (1986 <= y && y <= 2005)
    {
        double t = y - 2000;
        return 63.86 + 0.3345 * t - 0.060374 * pow(t, 2) + 0.0017275 * pow(t, 3) + 0.000651814 * pow(t, 4) +
               0.00002373599 * pow(t, 5);
    }
    else if (1961 <= y && y <= 1986)
    {
        double t = y - 1975;
        return 45.45 + 1.067 * t - pow(t,2) / 260 - pow(t, 3) / 718;
    }
    else if (1941 <= y && y <= 1961)
    {
        double t = y - 1950;
        return 29.07 + 0.407 * t - pow(t, 2) / 233 + pow(t, 3) / 2547;
    }
    else if (1920 <= y && y <= 1941)
    {
        double t = y - 1920;
        return 21.20 + 0.84493 * t - 0.076100 * pow(t, 2) + 0.0020936 * pow(t, 3);
    }
    else if (1900 <= y && y <= 1920)
    {
        double t = y - 1900;
        return -2.79 + 1.494119 * t - 0.0598939 * pow(t, 2) + 0.0061966 * pow(t, 3) - 0.000197 * pow(t, 4);
    }
    else if (1860 <= y && y <= 1900)
    {
        double t = y - 1860;
        return 7.62 + 0.5737 * t - 0.251754 * pow(t, 2) + 0.01680668 * pow(t, 3) - 0.0004473624 * pow(t, 4) +
               pow(t, 5) / 233174;
    }
    else if (1800 <= y && y <= 1860)
    {
        double t = y - 1800;
        return 13.72 - 0.332447 * t + 0.0068612 * pow(t, 2) + 0.0041116 * pow(t, 3) - 0.00037436 * pow(t, 4) +
               0.0000121272 * pow(t, 5) - 0.0000001699 * pow(t, 6) + 0.000000000875 * pow(t, 7);
    }
    else if (1700 <= y && y <= 1800)
    {
        double t = y - 1700;
        return 8.83 + 0.1603 * t - 0.0059285 * pow(t, 2) + 0.00013336 * pow(t, 3) - pow(t, 4) / 1174000;
    }
    else if (1600 <= y && y <= 1700)
    {
        double t = y - 1600;
        return 120 - 0.9808 * t - 0.01532 * pow(t, 2) + pow(t, 3) / 7129;
    }
    else if (500 <= y && y <= 1600)
    {
        double u = (y - 1000) / 100;
        return 1574.2 - 556.01 * u + 71.23472 * pow(u, 2) + 0.319781 * pow(u, 3) - 0.8503463 * pow(u, 4) -
               0.005050998 * pow(u, 5) + 0.0083572073 * pow(u, 6);
    }
    else if (-500 <= y && y <= 500)
    {
        double u = y / 100;
        return 10583.6 - 1014.41 * u + 33.78311 * pow(u, 2) - 5.952053 * pow(u, 3) - 0.1798452 * pow(u, 4) + \
               0.022174192 * pow(u, 5) + 0.0090316521 * pow(u, 6);
    }
    else    // y < -500
    {
        double u = (y - 1820) / 100;
        return -20 + 32 * pow(u, 2);
    }
}

//
// Solar calculator
// Results are passed by reference
// All calculations assume time inputs in Coordinated Universal Time (UTC)
//

// Calculate the Equation of (Ephemeris) Time, in minutes of time
//
void calcEquationOfTime(int year, int month, int day, int hour, int minute, int second, double &E, int sel)
{
    // Find Julian day
    double JD = calcJulianDay(year, month, day);
    double m = fractionalDay(hour, minute, second);
    double T = calcJulianCentSplit(JD, m);

    // Select method and write result, in minutes of time
    switch (sel)
    {
        case 1: default: E = equationOfTime1(T); break;
        case 2: E = equationOfTime2(T); break;
        case 3: E = equationOfTime3(T); break;
    }
}

// Calculate the Sun's right ascension and declination, in degrees
//
void calcEquatorialCoordinates(int year, int month, int day, int hour, int minute, int second,
                               double &rt_ascension, double &declination)
{
    // Find Julian day
    double JD = calcJulianDay(year, month, day);
    double m = fractionalDay(hour, minute, second);
    double T = calcJulianCentSplit(JD, m);

    // Write results, in degrees
    rt_ascension = calcSunRtAscension(T);
    declination = calcSunDeclination(T);
}

// Calculate the Sun's azimuth and elevation (altitude), in degrees
//
void calcHorizontalCoordinates(int year, int month, int day, int hour, int minute, int second,
                               double latitude, double longitude, double &azimuth, double &elevation)
{
    // Find Julian day
    double JD = calcJulianDay(year, month, day);
    double m = fractionalDay(hour, minute, second);
    double T = calcJulianCentSplit(JD, m);

    // Find solar coordinates
    double alpha = calcSunRtAscension(T);
    double delta = calcSunDeclination(T);

    // Find apparent sidereal time at Greenwich
    double theta0 = calcApparentSiderealTime(JD);
    theta0 = calcSiderealTimeInstant(theta0, m);

    // Find local angle hour
    double H = theta0 + longitude - alpha;

    // Write results, in degrees
    azimuth = calcSolarAzimuth(H, delta, latitude);
    elevation = calcSolarElevation(H, delta, latitude);
    elevation += calcRefractionCorr(elevation);
}

// Calculate the Sun's radius vector (distance), in AUs
//
void calcSunRadiusVector(int year, int month, int day, int hour, int minute, int second, double &radius_vector)
{
    // Find Julian day
    double JD = calcJulianDay(year, month, day);
    double m = fractionalDay(hour, minute, second);
    double T = calcJulianCentSplit(JD, m);

    // Write result, in AUs
    radius_vector = calcSunRadVector(T);
}

// Calculate the Sun's times of rising, transit and setting, in fraction of days
//
// local = false:   Results are between 0 and 1, Universal Time.
// local = true:    Results can be less than 0 or greater than 1, Universal Time. (default)
//                  *** Use the default option if you intend to convert your results to local standard time. ***
//
void calcSunriseSunset(int year, int month, int day, double latitude, double longitude,
                       double &transit, double &sunrise, double &sunset, double altitude, bool local)
{
    // Find Julian day
    double JD = calcJulianDay(year, month, day);
    double T1 = calcJulianCent(JD - 1);
    double T2 = calcJulianCent(JD);
    double T3 = calcJulianCent(JD + 1);

    // Standard altitude of the Sun
    double h0 = altitude;

    // Find solar coordinates
    double alpha1 = calcSunRtAscension(T1);
    double delta1 = calcSunDeclination(T1);
    double alpha2 = calcSunRtAscension(T2);
    double delta2 = calcSunDeclination(T2);
    double alpha3 = calcSunRtAscension(T3);
    double delta3 = calcSunDeclination(T3);

    // Find apparent sidereal time at Greenwich
    double theta0 = calcApparentSiderealTime(JD);

    // Local angle hour at sunrise or sunset (NaN if body is circumpolar)
    double H0 = degrees(acos((sin(radians(h0)) - sin(radians(latitude)) * sin(radians(delta2))) /
                        (cos(radians(latitude)) * cos(radians(delta2)))));

    // 1. Calculate approximate times of transit, rising and setting
    //
    double m0 = between0And1((alpha2 - longitude - theta0) / 360);
    double m1 = (local) ? m0 - H0 / 360 : between0And1(m0 - H0 / 360);
    double m2 = (local) ? m0 + H0 / 360 : between0And1(m0 + H0 / 360);

    // 2. Correction of transit, rising and setting
    //
    // Find ΔT = TD - UT
    double delta_t = calcDeltaT(year, month);

    // Number of corrections (1 or 2 max)
    int num_corr = 2;
    for (int i = 0; i < num_corr; i++)
    {
        double n0 = m0 + delta_t / 86400;
        double transit_alpha = interpolateCoordinates(n0, alpha1, alpha2, alpha3);
        double theta00 = calcSiderealTimeInstant(theta0, m0);
        double transit_ha = theta00 + longitude - transit_alpha;
        double transit_corr = - transit_ha / 360;

        double n1 = m1 + delta_t / 86400;
        double rise_alpha = interpolateCoordinates(n1, alpha1, alpha2, alpha3);
        double rise_delta = interpolateCoordinates(n1, delta1, delta2, delta3);
        double theta01 = calcSiderealTimeInstant(theta0, m1);
        double rise_ha = theta01 + longitude - rise_alpha;
        double rise_elev = calcSolarElevation(rise_ha, rise_delta, latitude);
        double rise_corr = (rise_elev - h0) /
                           (360.0 * cos(radians(rise_delta)) * cos(radians(latitude)) * sin(radians(rise_ha)));

        double n2 = m2 + delta_t / 86400;
        double set_alpha = interpolateCoordinates(n2, alpha1, alpha2, alpha3);
        double set_delta = interpolateCoordinates(n2, delta1, delta2, delta3);
        double theta02 = calcSiderealTimeInstant(theta0, m2);
        double set_ha = theta02 + longitude - set_alpha;
        double set_elev = calcSolarElevation(set_ha, set_delta, latitude);
        double set_corr = (set_elev - h0) /
                          (360.0 * cos(radians(set_delta)) * cos(radians(latitude)) * sin(radians(set_ha)));

        m0 = between0And1(m0 + transit_corr);
        m1 = (local) ? m1 + rise_corr : between0And1(m1 + rise_corr);
        m2 = (local) ? m2 + set_corr : between0And1(m2 + set_corr);
    }

    // Write final times, in fraction of days
    transit = m0;
    sunrise = m1;
    sunset = m2;
}

// Calculate the times of civil, nautical and astronomical dawn and dusk, in fraction of days
//
void calcCivilDawnDusk(int year, int month, int day, double latitude, double longitude,
                       double &transit, double &dawn, double &dusk)
{
    calcSunriseSunset(year, month, day, latitude, longitude,
                      transit, dawn, dusk, CIVIL_DAWNDUSK_STD_ALTITUDE);
}

void calcNauticalDawnDusk(int year, int month, int day, double latitude, double longitude,
                          double &transit, double &dawn, double &dusk)
{
    calcSunriseSunset(year, month, day, latitude, longitude,
                      transit, dawn, dusk, NAUTICAL_DAWNDUSK_STD_ALTITUDE);
}

void calcAstronomicalDawnDusk(int year, int month, int day, double latitude, double longitude,
                              double &transit, double &dawn, double &dusk)
{
    calcSunriseSunset(year, month, day, latitude, longitude,
                      transit, dawn, dusk, ASTRONOMICAL_DAWNDUSK_STD_ALTITUDE);
}
