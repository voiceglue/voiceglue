
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * Vocalocity, the Vocalocity logo, and VocalOS are trademarks or 
 * registered trademarks of Vocalocity, Inc. 
 * OpenVXI is a trademark of Scansoft, Inc. and used under license 
 * by Vocalocity.
 ***********************************************************************/

#include "DialogEventCounter.hpp"

void DialogEventCounter::Clear( bool clearForm )
{
	if ( clearForm )
	{
		formCounts.Clear();
		phaseName.erase();
	}

	fieldCounts.clear();
}

void DialogEventCounter::Clear( const VXMLElement &field )
{
	vxistring fieldName;
	field.GetAttribute(ATTRIBUTE__ITEMNAME, fieldName);
	Clear( fieldName );
}

void DialogEventCounter::Clear( const vxistring &fieldName )
{
	if ( !fieldName.empty() )
	{
		FIELDS::iterator i = fieldCounts.find( fieldName );
		if (i != fieldCounts.end())
			(*i).second.Clear();
	}
}

void DialogEventCounter::ClearIf( const vxistring & phase, bool condition )
{ 
	if ( ( phase == phaseName ) == condition ) 
	{ 
		Clear();
		phaseName = phase; 
	} 
}

void DialogEventCounter::Increment( const VXMLElement &item, const vxistring &eventName )
{
	// form-level <filled> elements increment the form-level event count...
	VXMLElementType name = item.GetName();
	if ( name == NODE_FILLED )
	{
		const VXMLElement &parent = item.GetParent();
		if ( parent.GetName() == NODE_FORM )
		{
			formCounts.Increment( eventName );
			return;
		}
	}

	// ... otherwise, increment the field level count.
	vxistring fieldName;
	item.GetAttribute(ATTRIBUTE__ITEMNAME, fieldName);

	if ( !fieldName.empty() )
	{
		FIELDS::iterator i = fieldCounts.find( fieldName );
		if (i != fieldCounts.end())
			(*i).second.Increment( eventName );
		else 
		{
			EventCounter field;
			field.Increment( eventName );
			fieldCounts[fieldName] = field;
		}
	}
}

int DialogEventCounter::GetCount( const VXMLElement &item, const vxistring &eventName )
{
	int result = 1;

	// form-level <filled> elements get the form-level event count...
	if ( item.GetName() == NODE_FILLED )
	{
		const VXMLElement &parent = item.GetParent();
		if ( parent.GetName() == NODE_FORM )
			return formCounts.GetCount( eventName );
	}

	// ... otherwise, get the field level count.
	vxistring fieldName;
	item.GetAttribute(ATTRIBUTE__ITEMNAME, fieldName);

	if ( !fieldName.empty() )
	{
		FIELDS::iterator i = fieldCounts.find( fieldName );
		if (i != fieldCounts.end())
			result = (*i).second.GetCount( eventName );
	}

	return result;
}
