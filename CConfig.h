// Handling for
// 1. Command-line options
// 2. Config-file options

#ifndef __CCONFIG_H__
#define __CCONFIG_H__

#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>

// tinyXml headers
#define TIXML_USE_STL
#include <tinyxml.h>

// ---------------------------------------------------------
// Support Class for automatic output type conversion
// ---------------------------------------------------------
class CValueConverter
{
    // Default convertion template function (if specific convertion does not exist)
public:
    template <class RETURNED_TYPE>
    bool convert(const std::string &, RETURNED_TYPE &) { return false; };
#ifdef xWIN32x
    template<> bool convert<std::string>(const std::string &input, std::string &output);
    template<> bool convert<uint64_t>(const std::string &input, uint64_t &output);
    template<> bool convert<uint32_t>(const std::string &input, uint32_t &output);
    template<> bool convert<uint16_t>(const std::string &input, uint16_t &output);
    template<> bool convert<uint8_t>(const std::string &input, uint8_t &output);
    template<> bool convert<int>(const std::string &input, int &output);
    template<> bool convert<size_t>(const std::string &input, size_t &output);
    template<> bool convert<bool>(const std::string &input, bool &output);
    template<> bool convert<std::vector<uint16_t>>(const std::string &input, std::vector<uint16_t> &output);
#endif
};

// ---------------------------------------------------------
// Class for configuration parameters management
// - parsing config (XML) file
// - parsing command line option
// - handling options set by code
// ---------------------------------------------------------
class CConfigMgr
{
public:
    CConfigMgr() {};
    virtual ~CConfigMgr() {};
    
    // Load xml file and indexes it with a configuration alias name
    bool loadConfigFile(const std::string &file_name );

    // Set default values of config section with map(<std::string, std::string>)
    void setDefaultValues( const std::string &config_section, std::map<std::string, std::string> in_default_values );

    // -------------------------------------------------------------------------
    // Get and store (output) a value ( parameter_name ) from an xml file (configuration_alias)
    // performing type convertion:
    // Example:
    //   getValue<std::string>("section_one", "param_one", param1);
    template <class RETURNED_TYPE>
    bool getValue( const std::string &confg_section, const std::string &parameter_name, RETURNED_TYPE &output )
    {
        bool rv( false );
    
        if ( getValueFromCustom<RETURNED_TYPE> ( confg_section, parameter_name, output ) )
            rv = true;
        else if ( getValueFromXmlDocument<RETURNED_TYPE>( confg_section, parameter_name, output ) )
            rv = true;
        else if ( getValueFromDefault<RETURNED_TYPE>( confg_section, parameter_name, output ) )
            rv = true;
    
        return rv;
    };
    

    // Set custom Values ( from command line argument )
    void setValue( const std::string &configuration_alias, const std::string &parameter_name, std::string input );

    // Print configuration
    friend std::ostream& operator<< (std::ostream& out, CConfigMgr const& cfg);

private:
    std::map< std::string, std::map< std::string, std::string > >    default_values;
    std::map< std::string, std::map< std::string, std::string > >    custom_values;
    typedef std::map< std::string, TiXmlElement* > SECTIONS_MAP;
    SECTIONS_MAP xml_sections;
    // xml doc 
    boost::shared_ptr<TiXmlDocument> m_xml_doc;

    boost::mutex m_lock;

    // -----------------------------------------------------------
    template <class RETURNED_TYPE>
    bool getValueFromXmlDocument( const std::string &confg_section, const std::string &parameter_name, RETURNED_TYPE &output )
    {
        if ( !xml_sections.count(confg_section) ) 
        {
            return false;
        }
    
        TiXmlElement* hRoot = xml_sections[confg_section];
        if ( hRoot->FirstChild(parameter_name) )
        {
            std::string output_text = hRoot->FirstChildElement(parameter_name)->GetText();
            return vconv.convert<RETURNED_TYPE>( output_text, output );
        }
    
        return false;
    }
    
    template <class RETURNED_TYPE>
    bool getValueFromDefault( const std::string &confg_section, const std::string &parameter_name, RETURNED_TYPE &output )
    {
        if ( default_values.count(confg_section) && default_values[confg_section].count(parameter_name) )
        {
            return vconv.convert<RETURNED_TYPE>( default_values[confg_section][parameter_name], output );
        }
        else
        {
            return false;
        }
    }
    
    template <class RETURNED_TYPE>
    bool getValueFromCustom( const std::string &confg_section, const std::string &parameter_name, RETURNED_TYPE &output )
    {
        if ( custom_values.count(confg_section) && custom_values[confg_section].count(parameter_name) )
        {
            return vconv.convert<RETURNED_TYPE>( custom_values[confg_section][parameter_name], output );
        }
        else
        {
            return false;
        }
    }
    
    // Value converter instance
    CValueConverter vconv;
};

// ---------------------------------------------------------
// Templates for automatic output type conversion
template<> 
inline bool CValueConverter::convert<std::string>(const std::string &input, std::string &output) { output = input; return true; };

template<>
inline bool CValueConverter::convert<uint64_t>(const std::string &input, uint64_t &output) { output = strtoul(input.c_str(), NULL, 0); return true; };

template<>
inline bool CValueConverter::convert<uint32_t>(const std::string &input, uint32_t &output) { output = strtoul(input.c_str(), NULL, 0); return true; };

template<>
inline bool CValueConverter::convert<uint16_t>(const std::string &input, uint16_t &output) { output = static_cast<uint16_t>(strtoul(input.c_str(), NULL, 0)); return true; };

template<>
inline bool CValueConverter::convert<uint8_t>(const std::string &input, uint8_t &output) { output = static_cast<uint8_t>(strtoul(input.c_str(), NULL, 0)); return true; };

template<>
inline bool CValueConverter::convert<int>(const std::string &input, int &output) { output = atoi(input.c_str()); return true; };

#if !defined(__GNUC__) && !defined(_MSC_VER)  //Hack to remove duplicate definitions since uint64_t and size_t are the same on linux.  Should be done more cleanly
template<>
inline bool CValueConverter::convert<size_t>(const std::string &input, size_t &output) { output = atoi(input.c_str()); return true; };
#endif

template<>
inline bool CValueConverter::convert<bool>(const std::string &input, bool &output) 
{
    if ( input == "true" || input == "True" || input == "TRUE" || input == "1" )
    {
        output = true;
        return true;
    }
    else if ( input == "false" || input == "False" || input == "FALSE" || input == "0" )
    {
        output = false;
        return true;
    }

    return false;
};

// parse string of "x,y,z" to vector<int>
template<>
inline bool CValueConverter::convert<std::vector<uint16_t> >(const std::string &input, std::vector<uint16_t> &output) 
{ 
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(", \t"); // separators here
    tokenizer tokens(input, sep);

    for (tokenizer::iterator tok_iter = tokens.begin();
           tok_iter != tokens.end(); ++tok_iter)
    {
          output.push_back(static_cast<uint16_t>(strtoul((*tok_iter).c_str(), NULL, 0)));
    }
    return true; 
};


#endif // __CCONFIG_H__
