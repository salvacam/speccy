/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2010 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "options.h"
#include "../platform/platform.h"
#include "../speccy.h"

#ifdef USE_CONFIG
#include <tinyxml2.h>
#include "../platform/io.h"
#endif//USE_CONFIG

namespace xOptions
{

eOptionB* eOptionB::Find(const char* name)
{
	for(eOptionB* o = First(); o; o = o->Next())
	{
		if(!strcmp(name, o->Name()))
			return o;
	}
	return NULL;
}
void eOptionInt::Change(int f, int l, bool next)
{
	if(next)
	{
		Set(self + 1);
		if(self >= l)
			Set(f);
	}
	else
	{
		Set(self - 1);
		if(self < f)
			Set(l - 1);
	}
}
const char*	eOptionInt::Value() const
{
	const char** vals = Values();
	if(!vals)
		return NULL;
	return vals[value];
}
void eOptionInt::Value(const char* v)
{
	const char** vals = Values();
	if(!vals)
		return;
	int i = -1;
	for(; *vals; ++vals)
	{
		++i;
		if(!strcmp(*vals, v))
		{
			value = i;
			break;
		}
	}
}
const char*	eOptionBool::Value() const
{
	const char** vals = Values();
	if(!vals)
		return NULL;
	return vals[value ? 1 : 0];
}
void eOptionBool::Value(const char* v)
{
	const char** vals = Values();
	if(!vals)
		return;
	if(!strcmp(v, vals[0]))
		value = false;
	else if(!strcmp(v, vals[1]))
		value = true;
}
const char** eOptionBool::Values() const
{
	static const char* values[] = { "off", "on", NULL };
	return values;
}
void eOptionString::Value(const char* v)
{
	int s = strlen(v) + 1;
	if(!value || alloc_size < s)
	{
		SAFE_DELETE_ARRAY(value);
		value = new char[s];
		alloc_size = s;
	}
	strcpy(const_cast<char*>(value), v);
}

struct eOA : public eOptionB // access to protected members hack
{
	static void SortByOrder()
	{
		bool swapped;
		do
		{
			swapped = false;
			for(eOptionB* a = First(), *pa = NULL; a; pa = a, a = a->Next())
			{
				for(eOptionB* b = a->Next(), *pb = a; b; pb = b, b = b->Next())
				{
					if(b->Order() < a->Order())
					{
						Swap((eOA*)pa, (eOA*)a, (eOA*)pb, (eOA*)b);
						swapped = true;
						break;
					}
				}
				if(swapped)
					break;
			}
		} while(swapped);
	}
	static void Swap(eOA* pa, eOA* a, eOA* pb, eOA* b)
	{
		eOptionB* n = a->next;
		a->next = b->next;
		if(a != pb)
			b->next = n;
		else
			b->next = a;
		if(pa)
			pa->next = b;
		else
			_First() = b;
		if(a != pb)
			pb->next = a;
	}
};

static void Apply()
{
	for(eOptionB* o = eOptionB::First(); o; o = o->Next())
	{
		o->Apply();
	}
}

#ifdef USE_CONFIG
static const char* FileName() { return xIo::ProfilePath("unreal_speccy_portable.xml"); }

static char buf[256];
static const char* OptNameToXmlName(const char* name)
{
	strcpy(buf, name);
	for(char* b = buf; *b; ++b)
	{
		if(*b == ' ')
			*b = '_';
	}
	return buf;
}
static const char* XmlNameToOptName(const char* name)
{
	strcpy(buf, name);
	for(char* b = buf; *b; ++b)
	{
		if(*b == '_')
			*b = ' ';
	}
	return buf;
}

//static const 

static const char StringToChar(const char* nameXml)
{
	if (strcmp(nameXml, "Cs") == 0) return 'c';
	if (strcmp(nameXml, "Ss") == 0) return 's';
	if (strcmp(nameXml, "Sp") == 0) return ' ';
	if (strcmp(nameXml, "En") == 0) return 'e';
	return *nameXml;
}

static const char* zx_keys[] =
{
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
	"A", "S", "D", "F", "G", "H", "J", "K", "L",
		"Z", "X", "C", "V", "B", "N", "M"
};

static const char zx_chars[] =
{
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
		'Z', 'X', 'C', 'V', 'B', 'N', 'M'
};

static const char* CharToString(const char name)
{
	if (name == 'c') return "Cs";
	if (name == 's') return "Ss";
	if (name == ' ') return "Sp";
	if (name == 'e') return "En";
	for(unsigned int i = 0 ; i < sizeof(zx_keys); i++)
	{
		if (name == zx_chars[i]) return zx_keys[i];
	}
	const char* result = &name;
	return result;
}

//=============================================================================
//	Load
//-----------------------------------------------------------------------------
void Load()
{
	using namespace tinyxml2;
	eOA::SortByOrder();
	XMLDocument doc;
	if(doc.LoadFile(FileName()) == XML_SUCCESS)
	{
		XMLElement* root = doc.RootElement();
		if(root)
		{
			XMLElement* opts = root->FirstChildElement("Options")->FirstChildElement();
			for(; opts; opts = opts->NextSiblingElement())
			{
				eOptionB* o = eOptionB::Find(XmlNameToOptName(opts->Value()));
				if(!o)
					continue;
				const char* v = opts->GetText();
				o->Value(v ? v : "");
			}
		}
	}
	Apply();
}

void LoadConfig(char* file)
{
	using namespace tinyxml2;
	XMLDocument doc;
	if(doc.LoadFile(file) == XML_SUCCESS) //TODO change filename
	{
		XMLElement* root = doc.RootElement();
		if(root)
		{
			XMLElement* opts = root->FirstChildElement("Control_mapping")->FirstChildElement();
			if(opts)
			{
				for(int i = 0; opts; opts = opts->NextSiblingElement())
				{
					kCustom[i] = StringToChar(opts->GetText());
					++i;
				}
			}

			//Load fullscreen
			XMLElement* fullscreen = root->FirstChildElement("Fullscreen");
			const char n = 'n';
			const char nMayus = 'N';
			if (fullscreen)
			{
				#ifdef V90
				eOptionB* oScreen = eOptionB::Find("fullscreen (L2 L+A)");
				#else
				eOptionB* oScreen = eOptionB::Find("fullscreen");			
				#endif

				const char* v = fullscreen->GetText();
				if(v[1] == n || v[1] == nMayus)
				{
					SAFE_CALL(oScreen)->Change();
				}
				oScreen->Value(v ? v : "");
			}

			//Load border
			XMLElement* border = root->FirstChildElement("Border");
			if (border)
			{
				eOptionB* oBorder = eOptionB::Find("border (L2 L+X)");
				const char* v = border->GetText();
				oBorder->Value(v ? v : "");
				oBorder->Apply();
			}

			//Load joystick
			XMLElement* joystick = root->FirstChildElement("Joystick");
			//static const char* values[] = { "kempston", "cursor", "qaop", "sinclair2", "custom", NULL };
			if (joystick)
			{
				eOptionB* o = eOptionB::Find("joystick");
				const char* v = joystick->GetText();
				o->Value(v ? v : "");
			}
		}
	}
	else
	{
		char kCustomOriginal[10] = {'O','P','Q','A','M',' ','1','2','0','3'}; /* {<, >, ^, v, B, A, Y, X, L, R} */
		for(unsigned int i = 0 ; i < sizeof(kCustomOriginal); i++)
		{
			kCustom[i] = kCustomOriginal[i];
		}
	}
}

//=============================================================================
//	Store
//-----------------------------------------------------------------------------
void Store()
{
	using namespace tinyxml2;
	XMLDocument doc;
	XMLDeclaration* decl = doc.NewDeclaration();
	doc.LinkEndChild(decl);
	XMLElement* root = doc.NewElement("UnrealSpeccyPortable");
	doc.LinkEndChild(root);
	XMLElement* opts = doc.NewElement("Options");
	root->LinkEndChild(opts);

	for(eOptionB* o = eOptionB::First(); o; o = o->Next())
	{
		if(!o->Storeable())
			continue;
		XMLElement* msg;
		msg = doc.NewElement(OptNameToXmlName(o->Name()));
		msg->LinkEndChild(doc.NewText(o->Value()));
		opts->LinkEndChild(msg);
	}
	doc.SaveFile(FileName());
}

void StoreConfig(char* file)
{
	using namespace tinyxml2;
	XMLDocument doc;
	XMLDeclaration* decl = doc.NewDeclaration();
	doc.LinkEndChild(decl);
	XMLElement* root = doc.NewElement("UnrealSpeccyPortable");
	doc.LinkEndChild(root);
	XMLElement* opts = doc.NewElement("Control_mapping");
	root->LinkEndChild(opts);


	const char* joystick[10] = { "Left", "Rigth", "Up", "Down", "B", "A", "X", "Y", "L", "R"};

	for(int i = 0 ; i < 10; i++)
	{
		XMLElement* msg;
		msg = doc.NewElement(joystick[i]);
		msg->LinkEndChild(doc.NewText( CharToString(kCustom[i]) ) );
		opts->LinkEndChild(msg);
	}

	//save fullscreen
	XMLElement* fullScreen;
	fullScreen = doc.NewElement("Fullscreen");
	#ifdef V90
	eOptionB* oScreen = eOptionB::Find("fullscreen (L2 L+A)");
	#else
	eOptionB* oScreen = eOptionB::Find("fullscreen");			
	#endif
	fullScreen->LinkEndChild(doc.NewText( oScreen->Value() ) );
	root->LinkEndChild(fullScreen);

	//save border
	XMLElement* borderSave = doc.NewElement("Border");
	eOptionB* oBorder = eOptionB::Find("border (L2 L+X)");
	borderSave->LinkEndChild(doc.NewText( oBorder->Value() ) );
	root->LinkEndChild(borderSave);	

	//save joystick type
	XMLElement* joystickSave = doc.NewElement("Joystick");
	eOptionB* o = eOptionB::Find("joystick");
	joystickSave->LinkEndChild(doc.NewText( o->Value() ) );
	root->LinkEndChild(joystickSave);	

	doc.SaveFile(file);
}

#else//USE_CONFIG

void Load() { eOA::SortByOrder(); Apply(); }
void Store() {}

#endif//USE_CONFIG

}
//namespace xOptions
