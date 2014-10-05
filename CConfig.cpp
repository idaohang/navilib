// Handling for
// 1. Command-line options
// 2. Config-file options

#include "CConfig.h"

// -----------------------------
// Load xml file and index it
bool CConfigMgr::loadConfigFile(const std::string &file_name )
{
    boost::mutex::scoped_lock lock(m_lock);
    std::string section_name;

    m_xml_doc = boost::shared_ptr<TiXmlDocument>(new TiXmlDocument());
    if ( !m_xml_doc->LoadFile(file_name.c_str()) )
    {
        std::cout << m_xml_doc->ErrorDesc() << std::endl;
        return false;
    }

    // get an handle to the document sections level
    TiXmlElement* pElem; // (m_xml_doc->RootElement()->FirstChild());

    // 2nd level - <title> / <section>
    for(pElem = m_xml_doc->RootElement()->FirstChildElement(); pElem; pElem = pElem->NextSiblingElement() )
    {
        // get section name
        section_name = pElem->Value();

        // store the xml in the map
        xml_sections[section_name] = pElem;
    }
    return true;
};

// -------------------------------------------------------------------------
// Set default values of config section with map(<std::string, std::string>)
void CConfigMgr::setDefaultValues( const std::string &config_section, std::map<std::string, std::string> in_default_values ) 
{ 
    boost::mutex::scoped_lock lock(m_lock);
    // create new map for the configuration section if no one is present
    if ( !default_values.count(config_section) )
    {
        default_values.insert( make_pair( config_section, std::map<std::string, std::string>() ) );
    }
    // copy the map
    default_values[config_section].insert( in_default_values.begin(), in_default_values.end() );
};

// -------------------------------------------------------------------------
// Set custom Values ( from command line argument )
void CConfigMgr::setValue( const std::string &confg_section, const std::string &parameter_name, std::string input )
{
    boost::mutex::scoped_lock lock(m_lock);

    if ( !custom_values.count(confg_section) )
    {
        custom_values.insert( make_pair( confg_section, std::map<std::string, std::string>() ) );
    }	

    custom_values[confg_section].insert( make_pair( parameter_name, input ) );
}

// -------------------------------------------------------------------------
// print configuration to stream
std::ostream& operator<< (std::ostream& out, CConfigMgr const& Cfg)
{
       using namespace std;
       CConfigMgr::SECTIONS_MAP::const_iterator itr;    
   
       out << " -- Configuration --" << endl;
   
       for(itr = Cfg.xml_sections.begin(); itr != Cfg.xml_sections.end(); itr++)
       {
           out << itr->first << ": " << endl;
   
           TiXmlElement* pElem;
           for(pElem = itr->second->FirstChildElement(); pElem; pElem = pElem->NextSiblingElement() )
           {
               std::string par_val = pElem->GetText();
               std::string par_name  = pElem->Value();
               out << "  " << par_name << " = " << par_val << endl;
           }
           out << endl;
       }
       return out;
}


