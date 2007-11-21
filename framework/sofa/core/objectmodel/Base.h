/*******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 beta 1       *
*                (c) 2006-2007 MGH, INRIA, USTL, UJF, CNRS                     *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Contact information: contact@sofa-framework.org                              *
*                                                                              *
* Authors: J. Allard, P-J. Bensoussan, S. Cotin, C. Duriez, H. Delingette,     *
* F. Faure, S. Fonteneau, L. Heigeas, C. Mendoza, M. Nesme, P. Neumann,        *
* and F. Poyer                                                                 *
*******************************************************************************/
#ifndef SOFA_CORE_OBJECTMODEL_BASE_H
#define SOFA_CORE_OBJECTMODEL_BASE_H

#include <sofa/helper/system/config.h>
#include <sofa/core/objectmodel/DataPtr.h>
#include <sofa/core/objectmodel/Data.h>
#include <sofa/core/objectmodel/BaseObjectDescription.h>
#include <string>

using sofa::core::objectmodel::DataPtr;
using sofa::core::objectmodel::Data;

namespace sofa
{

namespace core
{

namespace objectmodel
{

/**
 *  \brief Base class for everything
 *
 *  This class contains all functionnality shared by every objects in SOFA.
 *  Most importantly it defines how to retrieve information about an object (name, type, fields).
 *
 */
class Base
{
public:
    Base();
    virtual ~Base();

    /// Name of the object.
    Data<std::string> name;

    /// Accessor to the object name
    std::string getName() const;

    /// Set the name of this object
    void setName(const std::string& n);

    /// Get the type name of this object (i.e. class and template types)
    virtual std::string getTypeName() const
    {
        return decodeTypeName(typeid(*this));
    }

    /// Get the class name of this object
    virtual std::string getClassName() const
    {
        return decodeClassName(typeid(*this));
    }

    /// Get the template type names (if any) used to instantiate this object
    virtual std::string getTemplateName() const
    {
        return decodeTemplateName(typeid(*this));
    }

    /// Helper method to decode the type name to a more readable form if possible
    static std::string decodeTypeName(const std::type_info& t);

    /// Helper method to extract the class name (removing namespaces and templates)
    static std::string decodeClassName(const std::type_info& t);

    /// Helper method to extract the namespace (removing class name and templates)
    static std::string decodeNamespaceName(const std::type_info& t);

    /// Helper method to extract the template name (removing namespaces and class name)
    static std::string decodeTemplateName(const std::type_info& t);

    /// Helper method to get the type name of a type derived from this class
    ///
    /// This method should be used as follow :
    /// \code  T* ptr = NULL; std::string type = T::typeName(ptr); \endcode
    /// This way derived classes can redefine the typeName method
    template<class T>
    static std::string typeName(const T* = NULL)
    {
        return decodeTypeName(typeid(T));
    }

    /// Helper method to get the class name of a type derived from this class
    ///
    /// This method should be used as follow :
    /// \code  T* ptr = NULL; std::string type = T::className(ptr); \endcode
    /// This way derived classes can redefine the className method
    template<class T>
    static std::string className(const T* = NULL)
    {
        return decodeClassName(typeid(T));
    }

    /// Helper method to get the namespace name of a type derived from this class
    ///
    /// This method should be used as follow :
    /// \code  T* ptr = NULL; std::string type = T::namespaceName(ptr); \endcode
    /// This way derived classes can redefine the namespaceName method
    template<class T>
    static std::string namespaceName(const T* = NULL)
    {
        return decodeNamespaceName(typeid(T));
    }

    /// Helper method to get the template name of a type derived from this class
    ///
    /// This method should be used as follow :
    /// \code  T* ptr = NULL; std::string type = T::templateName(ptr); \endcode
    /// This way derived classes can redefine the templateName method
    template<class T>
    static std::string templateName(const T* = NULL)
    {
        return decodeTemplateName(typeid(T));
    }

    /// Assign the field values stored in the given list of name + value pairs of strings
    void parseFields ( std::list<std::string> str );

    /// Assign the field values stored in the given map of name -> value pairs
    virtual void parseFields ( const std::map<std::string,std::string*>& str );

    /// Write the current field values to the given map of name -> value pairs
    void writeDatas (std::map<std::string,std::string*>& str);

    /// Write the current field values to the given text output stream
    void writeDatas (std::ostream& out);

    /// Write the current Node values to the given XML output stream
    void xmlWriteNodeDatas (std::ostream& out, unsigned level);

    /// Write the current field values to the given XML output stream
    void xmlWriteDatas (std::ostream& out, unsigned level);

    /// Find a field give its name, if not found, the index is the size of the vector
    unsigned int findField( const char* name ) const
    {
        std::string ln(name);
        unsigned int i;
        for ( i=0; i<m_fieldVec.size(); i++)
        {
            if (m_fieldVec[i].first == ln) return i;
        }
        return i;
    }
    unsigned int findField( const std::string &name ) const
    {
        unsigned int i;
        for ( i=0; i<m_fieldVec.size(); i++)
        {
            if (m_fieldVec[i].first == name) return i;
        }
        return i;
    }
    /// Helper method used to initialize a field containing a value of type T
    template<class T>
    Data<T> initData( Data<T>* field, const char* name, const char* help )
    {
        std::string ln(name);
        if( ln.size()>0 && findField(ln)!=m_fieldVec.size() )
        {
            std::cerr << "field name " << ln << " already used in this class or in a parent class !...aborting" << std::endl;
            exit( 1 );
        }
        //field = tmp;
        m_fieldVec.push_back( std::make_pair(ln,field));
        return Data<T>(help);
    }

    /// Helper method used to initialize a field containing a value of type T
    template<class T>
    Data<T> initData( Data<T>* field, const T& value, const char* name, const char* help )
    {
        std::string ln(name);
        if( ln.size()>0 && findField(ln)!=m_fieldVec.size()  )
        {
            std::cerr << "field name " << ln << " already used in this class or in a parent class !...aborting" << std::endl;
            exit( 1 );
        }
        //field = tmp;
        m_fieldVec.push_back( std::make_pair(ln,field));
        return Data<T>(value,help);
    }

    /// Helper method used to initialize a field pointing to a value of type T
    template<class T>
    DataPtr<T> initDataPtr( DataPtr<T>* field, T* ptr, const char* name, const char* help )
    {
        std::string ln(name);
        if( ln.size()>0 && findField(ln)!=m_fieldVec.size() )
        {
            std::cerr << "field name " << ln << " already used in this class or in a parent class !...aborting" << std::endl;
            exit( 1 );
        }
        //field = tmp;
        m_fieldVec.push_back( std::make_pair(ln,field));
        return DataPtr<T>(ptr,help);
    }


    /// Parse the given description to assign values to this object's fields and potentially other parameters
    virtual void parse ( BaseObjectDescription* arg );

    /// Accessor to the map containing all the fields of this object
    std::vector< std::pair<std::string, BaseData*> > getFields() { return m_fieldVec; }

protected:
    /// name -> Field object
    std::vector< std::pair<std::string, BaseData*> > m_fieldVec;

    /// Add a field. Note that this method should only be called if the field was not initialized with the initData<T> of field<T> methods
    void addField( BaseData* f, const char* name )
    {
        std::string ln(name);
        if( ln.size()>0 && findField(ln)!=m_fieldVec.size() )
        {
            std::cerr << "field name " << ln << " already used in this class or in a parent class !...aborting" << std::endl;
            exit( 1 );
        }
        m_fieldVec.push_back( std::make_pair(ln,f));
    }
};

} // namespace objectmodel

} // namespace core

} // namespace sofa

#endif

