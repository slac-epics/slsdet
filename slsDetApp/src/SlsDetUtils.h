#ifndef SlsDetUtils_H
#define SlsDetUtils_H

#include <string>
#include <map>

namespace SlsDetUtils {

enum Feature {
  DETSTATUS,
  NEXTFRAME,
  HOSTNAME,
  DETTYPE,
  MODULEID,
  SERIALNUM,
  FIRMWARE,
  SOFTWARE,
  HARDWARE,
  KERNELVER,
  CHIPVER,
  FPGATEMP,
  ADCTEMP,
  THRESHOLD,
  TEMPCONTROL,
  TEMPEVENT,
  CHIPPOWER,
  HIGHVOLTAGE,
  CLOCKSPEED,
  GAINMODE,
  HIGHGAIN0,
  ADCPHASE,
  EXPTIME,
  PERIOD,
  DELAY,
  TRIGMODE,
  NUMFRAMES,
  NUMTRIG,
  NUMINTFACE,
  ACTINTFACE,
  SRCUDPIP,
  SRCUDPIP2,
  SRCUDPMAC,
  SRCUDPMAC2,
  DESTUDPIP,
  DESTUDPIP2,
  DESTUDPMAC,
  DESTUDPMAC2,
  DESTUDPPORT,
  DESTUDPPORT2,
  TXDELAYFRAME,
  FLOWCONTROL,
};

#define FEATURE_CASE(x) case x: return #x
inline std::string FeatureName(Feature feature)
{
  switch (feature) {
    FEATURE_CASE(DETSTATUS);
    FEATURE_CASE(NEXTFRAME);
    FEATURE_CASE(HOSTNAME);
    FEATURE_CASE(DETTYPE);
    FEATURE_CASE(MODULEID);
    FEATURE_CASE(SERIALNUM);
    FEATURE_CASE(FIRMWARE);
    FEATURE_CASE(SOFTWARE);
    FEATURE_CASE(HARDWARE);
    FEATURE_CASE(KERNELVER);
    FEATURE_CASE(CHIPVER);
    FEATURE_CASE(FPGATEMP);
    FEATURE_CASE(ADCTEMP);
    FEATURE_CASE(THRESHOLD);
    FEATURE_CASE(TEMPCONTROL);
    FEATURE_CASE(TEMPEVENT);
    FEATURE_CASE(CHIPPOWER);
    FEATURE_CASE(HIGHVOLTAGE);
    FEATURE_CASE(CLOCKSPEED);
    FEATURE_CASE(GAINMODE);
    FEATURE_CASE(HIGHGAIN0);
    FEATURE_CASE(ADCPHASE);
    FEATURE_CASE(EXPTIME);
    FEATURE_CASE(PERIOD);
    FEATURE_CASE(DELAY);
    FEATURE_CASE(TRIGMODE);
    FEATURE_CASE(NUMFRAMES);
    FEATURE_CASE(NUMTRIG);
    FEATURE_CASE(NUMINTFACE);
    FEATURE_CASE(ACTINTFACE);
    FEATURE_CASE(SRCUDPIP);
    FEATURE_CASE(SRCUDPIP2);
    FEATURE_CASE(SRCUDPMAC);
    FEATURE_CASE(SRCUDPMAC2);
    FEATURE_CASE(DESTUDPIP);
    FEATURE_CASE(DESTUDPIP2);
    FEATURE_CASE(DESTUDPMAC);
    FEATURE_CASE(DESTUDPMAC2);
    FEATURE_CASE(DESTUDPPORT);
    FEATURE_CASE(DESTUDPPORT2);
    FEATURE_CASE(TXDELAYFRAME);
    FEATURE_CASE(FLOWCONTROL);
    default: return "UNKNOWN";
  }
}
#undef FEATURE_CASE

template<typename T>
class FeatureInfo {
public:
  FeatureInfo(Feature feature, bool init, bool writable) :
    _init(init),
    _writable(writable),
    _request(false),
    _pending(false),
    _feature(feature),
    _value()
  {}
  virtual ~FeatureInfo() = default;

  bool isInitOnly() const { return _init; }
  bool isWritable() const { return _writable; }
  bool isRequested() const { return _request; }
  bool isPending() const { return _pending; }
  Feature feature() const { return _feature; }

  void setRequested() { _request = true; }
  void clearRequested() { _request = false; }

  void setPending() { _pending = true; }
  void clearPending() { _pending = false; }

  void write(const T& value) { _value = value; }

  T read() const { return _value; }

private:
  bool    _init;
  bool    _writable;
  bool    _request;
  bool    _pending;
  Feature _feature;
  T       _value;
};

typedef FeatureInfo<int> IntFeature;
typedef std::map<int, IntFeature> IntFeatureMap;
typedef FeatureInfo<double> DoubleFeature;
typedef std::map<int, DoubleFeature> DoubleFeatureMap;
typedef FeatureInfo<std::string> StringFeature;
typedef std::map<int, StringFeature> StringFeatureMap;

}

#endif
