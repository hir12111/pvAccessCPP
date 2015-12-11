/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <algorithm>

#include <pv/epicsException.h>

#include <osiSock.h>
#include <epicsStdlib.h>

#define epicsExportSharedSymbols
#include <pv/configuration.h>

#if defined(__GNUC__) && __GNUC__ < 3
#define OLDGCC
#define NO_STREAM_EXCEPTIONS
#endif

namespace epics {
namespace pvAccess {

using namespace epics::pvData;
using namespace std;

Properties::Properties() {}

Properties::Properties(const string &fileName) : _fileName(fileName) {}

const std::string &Properties::getProperty(const string &key) const
{
    _properties_t::const_iterator propertiesIterator = _properties.find(key);
    if(propertiesIterator != _properties.end()) {
        return propertiesIterator->second;
    } else {
        THROW_BASE_EXCEPTION(string("Property not found in the map: ") + key);
	}
}

const std::string &Properties::getProperty(const string &key, const string &defaultValue) const
{
    _properties_t::const_iterator propertiesIterator = _properties.find(key);
    if(propertiesIterator != _properties.end()) {
        return propertiesIterator->second;
    } else {
        return defaultValue;
    }
}

void Properties::Properties::load()
{
    load(_fileName);
}

void Properties::load(const string &fileName)
{
    ifstream strm(fileName.c_str());
    load(strm);
}

namespace {
string trim(const string& in)
{
    size_t A = in.find_first_not_of(" \t\r"),
           B = in.find_last_not_of(" \t\r");
    if(A==B)
        return string();
    else
        return in.substr(A, B-A+1);
}
}

void Properties::load(std::istream& strm)
{
    _properties_t newmap;

    std::string line;
    unsigned lineno = 0;
    while(getline(strm, line).good()) {
        lineno++;
        size_t idx = line.find_first_not_of(" \t\r");
        if(idx==line.npos || line[idx]=='#')
            continue;

        idx = line.find_first_of('=');
        if(idx==line.npos) {
            ostringstream msg;
            msg<<"Malformed line "<<lineno<<" expected '='";
            throw runtime_error(msg.str());
        }

        string key(trim(line.substr(0, idx))),
               value(trim(line.substr(idx+1)));

        if(key.empty()) {
            ostringstream msg;
            msg<<"Malformed line "<<lineno<<" expected name before '='";
            throw runtime_error(msg.str());
        }

        newmap[key] = value;
    }
    if(strm.bad()) {
        ostringstream msg;
        msg<<"Malformed line "<<lineno<<" I/O error";
        throw runtime_error(msg.str());
    }
    _properties.swap(newmap);
}

void Properties::store() const
{
    store(_fileName);
}

void Properties::store(const std::string& fname) const
{
    ofstream strm(fname.c_str());
    store(strm);
}

void Properties::store(std::ostream& strm) const
{
    for(_properties_t::const_iterator it=_properties.begin(), end=_properties.end();
        it!=end && strm.good(); ++it)
    {
        strm << it->first << " = " << it->second << "\n";
    }
}

void Properties::list()
{
	for (std::map<std::string,std::string>::iterator propertiesIterator =  _properties.begin() ;
			propertiesIterator != _properties.end();
			propertiesIterator++ )
	{
		cout << "Key:" << propertiesIterator->first << ",Value: " << propertiesIterator->second << endl;
	}
}

bool Configuration::getPropertyAsBoolean(const std::string &name, const bool defaultValue) const
{
    string value = getPropertyAsString(name, defaultValue ? "1" : "0");
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    bool isTrue = (value == "1") || (value == "true") || (value == "yes");
    if (isTrue)
        return true;

    bool isFalse = (value == "0") || (value == "false") || (value == "no");
    if (isFalse)
        return false;

    // invalid value
    return defaultValue;
}

epics::pvData::int32 Configuration::getPropertyAsInteger(const std::string &name, const epics::pvData::int32 defaultValue) const
{
    epicsInt32 ret;
    std::string val(getPropertyAsString(name, ""));

    if(epicsParseInt32(val.c_str(), &ret, 0, NULL))
        return defaultValue;
    return ret;
}

float Configuration::getPropertyAsFloat(const std::string &name, const float defaultValue) const
{
    float ret;
    std::string val(getPropertyAsString(name, ""));

    if(epicsParseFloat(val.c_str(), &ret, NULL))
        return defaultValue;
    return ret;
}

double Configuration::getPropertyAsDouble(const std::string &name, const double defaultValue) const
{
    double ret;
    std::string val(getPropertyAsString(name, ""));

    if(epicsParseDouble(val.c_str(), &ret, NULL))
        return defaultValue;
    return ret;
}

std::string Configuration::getPropertyAsString(const std::string &name, const std::string &defaultValue) const
{
    std::string val;
    if(tryGetPropertyAsString(name, &val))
        return val;
    else
        return defaultValue;
}

bool Configuration::getPropertyAsAddress(const std::string& name, osiSockAddr* addr) const
{
    unsigned short dftport=0;
    if(addr->sa.sa_family==AF_INET)
        dftport = ntohs(addr->ia.sin_port);

    std::string val(getPropertyAsString(name, ""));

    if(val.empty()) return false;

    addr->ia.sin_family = AF_INET;
    if(aToIPAddr(val.c_str(), dftport, &addr->ia))
        return false;
    return true;
}

bool Configuration::hasProperty(const std::string &name) const
{
    return tryGetPropertyAsString(name, NULL);
}

bool ConfigurationMap::tryGetPropertyAsString(const std::string& name, std::string* val) const
{
    properties_t::const_iterator it = properties.find(name);
    if(it==properties.end())
        return false;
    if(val)
        *val = it->second;
    return true;
}

bool ConfigurationEnviron::tryGetPropertyAsString(const std::string& name, std::string* val) const
{
    const char *env = getenv(name.c_str());
    if(!env || !*env)
        return false;
    if(val)
        *val = env;
    return true;
}

bool ConfigurationStack::tryGetPropertyAsString(const std::string& name, std::string* val) const
{
    for(confs_t::const_reverse_iterator it = confs.rbegin(), end = confs.rend();
        it!=end; ++it)
    {
        Configuration& conf = **it;
        if(conf.tryGetPropertyAsString(name, val))
            return true;
    }
    return false;
}

void ConfigurationProviderImpl::registerConfiguration(const string &name, Configuration::shared_pointer const & configuration)
{
	Lock guard(_mutex);
	std::map<std::string,Configuration::shared_pointer>::iterator configsIter = _configs.find(name);
	if(configsIter != _configs.end())
	{
		string msg = "configuration with name " + name + " already registered";
		THROW_BASE_EXCEPTION(msg.c_str());
	}
	_configs[name] = configuration;
}

Configuration::shared_pointer ConfigurationProviderImpl::getConfiguration(const string &name)
{
	std::map<std::string,Configuration::shared_pointer>::iterator configsIter = _configs.find(name);
	if(configsIter != _configs.end())
	{
		return configsIter->second;
	}
	return Configuration::shared_pointer();
}

ConfigurationProvider::shared_pointer configurationProvider;
Mutex conf_factory_mutex;

ConfigurationProvider::shared_pointer ConfigurationFactory::getProvider()
{
	Lock guard(conf_factory_mutex);
	if(configurationProvider.get() == NULL)
	{
		configurationProvider.reset(new ConfigurationProviderImpl());
		// default
		Configuration::shared_pointer systemConfig(new SystemConfigurationImpl());
		configurationProvider->registerConfiguration("system", systemConfig);
	}
	return configurationProvider;
}

}}

