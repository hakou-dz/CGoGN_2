/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
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
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#ifndef CORE_CONTAINER_CHUNK_ARRAY_CONTAINER_H_
#define CORE_CONTAINER_CHUNK_ARRAY_CONTAINER_H_

#include <core/basic/nameTypes.h>
#include <utils/assert.h>
#include <core/basic/dll.h>

#include <core/container/chunk_array.h>
#include <core/container/chunk_stack.h>
#include <core/container/chunk_array_factory.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <utils/assert.h>
#include <utils/make_unique.h>

namespace cgogn
{

class CGOGN_CORE_API ContainerBrowser
{
public:

	virtual unsigned int begin() const = 0;
	virtual unsigned int end() const = 0;
	virtual void next(unsigned int &it) const = 0;
	virtual void nextPrimitive(unsigned int &it, unsigned int primSz) const = 0;
	virtual void enable() = 0;
	virtual void disable() = 0;
	virtual ~ContainerBrowser();
};

template <typename CONTAINER>
class ContainerStandardBrowser : public ContainerBrowser
{
	const CONTAINER* cac_;

public:

	ContainerStandardBrowser(const CONTAINER* cac) : cac_(cac) {}
	virtual unsigned int begin() const { return cac_->realBegin(); }
	virtual unsigned int end() const { return cac_->realEnd(); }
	virtual void next(unsigned int &it)  const { cac_->realNext(it); }
	virtual void nextPrimitive(unsigned int &it, unsigned int primSz) const { cac_->realNextPrimitive(it,primSz); }
	virtual void enable() {}
	virtual void disable() {}
	virtual ~ContainerStandardBrowser() {}
};


/**
 * @brief class that manage the storage of several ChunkArray
 * @tparam CHUNKSIZE chunk size for ChunkArray
 * @detail
 *
 */
template <unsigned int CHUNKSIZE, typename T_REF>
class ChunkArrayContainer
{
public:

	/**
	* constante d'attribut inconnu
	*/
	static const unsigned int UNKNOWN = 0xffffffff;

protected:

	/**
	* vector of pointers to ChunkVector
	*/
	std::vector<ChunkArrayGen<CHUNKSIZE>*> table_arrays_;

	std::vector<std::string> names_;

	std::vector<std::string> type_names_;

	ChunkArray<CHUNKSIZE, T_REF> refs_;

	/**
	 * stack of holes
	 */
	ChunkStack<CHUNKSIZE, unsigned int> holes_stack_;

	/**
	* size (number of elts) of the container
	*/
	unsigned int nb_used_lines_;

	/**
	* size of the container with holes (also index of next inserted line if no holes)
	*/
	unsigned int nb_max_lines_;

	/**
	 * @brief number of bool attribs (which are alway in front of all others)
	 */
	unsigned int nb_marker_attribs_;

	/**
	 * Browser that allow special traversals
	 */
	ContainerBrowser* current_browser_;

	/**
	 * Browser that allow special traversals
	 */
	std::unique_ptr< ContainerStandardBrowser< ChunkArrayContainer<CHUNKSIZE, T_REF> > > std_browser_;

	/**
	 * @brief get array index from name
	 * @warning do not store index (not stable)
	 * @param attribName name of ChunkArray
	 * @return the index in table
	 */
	unsigned int getArrayIndex(const std::string& attribute_name) const
	{
		for (unsigned int i = 0; i != names_.size(); ++i)
		{
			if (names_[i] == attribute_name)
				return i;
		}
		return UNKNOWN;
	}

	/**
	 * @brief get array index from ptr
	 * @warning do not store index (not stable)
	 * @param ptr of ChunkArray
	 * @return the index in table
	 */
	unsigned int getArrayIndex(const ChunkArrayGen<CHUNKSIZE>* ptr) const
	{
		for (unsigned int i = 0u; i != table_arrays_.size(); ++i)
		{
			if (table_arrays_[i] == ptr)
				return i;
		}
		return UNKNOWN;
	}

	/**
	 * @brief remove an attribute by its name
	 * @param attribName name of attribute to remove
	 * @return true if attribute exist and has been removed
	 */
	bool removeAttribute(unsigned int index)
	{
		// store ptr for using it before delete
		ChunkArrayGen<CHUNKSIZE>* ptrToDel = table_arrays_[index];

		// in case of Marker attribute, keep Marker attributes first !
		if (index < nb_marker_attribs_)
		{
			nb_marker_attribs_--;

			if (index < nb_marker_attribs_)	// if attribute is not last of Markers
			{
				table_arrays_[index] = table_arrays_[nb_marker_attribs_];	// copy last of boolean on index
				names_[index]        = names_[nb_marker_attribs_];
				type_names_[index]   = type_names_[nb_marker_attribs_];
			}
			// now overwrite last of bool with last
			index = nb_marker_attribs_;
		}

		if (index != table_arrays_.size() - std::size_t(1u))
		{
			table_arrays_[index] = table_arrays_.back();
			names_[index]        = names_.back();
			type_names_[index]   = type_names_.back();
		}

		table_arrays_.pop_back();
		names_.pop_back();
		type_names_.pop_back();

		delete ptrToDel;

		return true;
	}

public:

	/**
	 * @brief ChunkArrayContainer constructor
	 */
	ChunkArrayContainer():
		nb_used_lines_(0u)
		,nb_max_lines_(0u)
		,std_browser_(make_unique< ContainerStandardBrowser<ChunkArrayContainer<CHUNKSIZE, T_REF>> >(this))
	{
		current_browser_= std_browser_.get();
	}

	ChunkArrayContainer(ChunkArrayContainer<CHUNKSIZE, T_REF>const& ) = delete;
	ChunkArrayContainer(ChunkArrayContainer<CHUNKSIZE, T_REF>&& ) = delete;
	ChunkArrayContainer& operator=(ChunkArrayContainer<CHUNKSIZE, T_REF>const& ) = delete;
	ChunkArrayContainer& operator=(ChunkArrayContainer<CHUNKSIZE, T_REF>&& ) = delete;

	/**
	 * @brief ChunkArrayContainer destructor
	 */
	~ChunkArrayContainer()
	{
		if (current_browser_ != std_browser_.get())
			delete current_browser_;
		for (auto ptr: table_arrays_)
			delete ptr;
	}

	/**
	 * @brief get an attribute
	 * @param attribute_name name of attribute
	 * @tparam T type of attribute
	 * @return pointer on attribute ChunkArray
	 */
	template <typename T>
	ChunkArray<CHUNKSIZE,T>* getAttribute(const std::string& attribute_name)
	{

		// first check if attribute already exist
		unsigned int index = getArrayIndex(attribute_name);
		if (index == UNKNOWN)
		{
			std::cerr << "attribute " << attribute_name << " not found." << std::endl;
			return nullptr;
		}

		return static_cast<ChunkArray<CHUNKSIZE, T>*>(table_arrays_[index]);
	}

	/**
	 * @brief add an attribute
	 * @param attribute_name name of attribute
	 * @tparam T type of attribute
	 * @return pointer on created attribute ChunkArray
	 */
	template <typename T>
	ChunkArray<CHUNKSIZE, T>* addAttribute(const std::string& attribute_name)
	{
		cgogn_assert(attribute_name.size() != 0);

		// first check if attribute already exist
		unsigned int index = getArrayIndex(attribute_name);
		if (index != UNKNOWN)
		{
			std::cerr << "attribute " << attribute_name << " already exists.." << std::endl;
			return nullptr;
		}

		// create the new attribute
		const std::string& typeName = nameOfType(T());
		ChunkArray<CHUNKSIZE, T>* carr = new ChunkArray<CHUNKSIZE,T>();
		ChunkArrayFactory<CHUNKSIZE>::template registerCA<T>();

		// reserve memory
		carr->setNbChunks(refs_.getNbChunks());

		// store pointer, name & typename.
		table_arrays_.push_back(carr);
		names_.push_back(attribute_name);
		type_names_.push_back(typeName);

		return carr ;
	}

	/**
	 * @brief add a Marker attribute
	 * @param attribute_name name of marker attribute
	 * @return pointer on created ChunkArray
	 */
	ChunkArray<CHUNKSIZE, bool>* addMarkerAttribute(const std::string& attribute_name)
	{
		ChunkArray<CHUNKSIZE, bool>* ptr = addAttribute<bool>(attribute_name);

		if (table_arrays_.size() > nb_marker_attribs_)
		{
			// swap ptrs
			auto tmp = table_arrays_.back();
			table_arrays_.back() = table_arrays_[nb_marker_attribs_];
			table_arrays_[nb_marker_attribs_] = tmp;
			// swap names & typenames
			names_.back().swap(names_[nb_marker_attribs_]);
			type_names_.back().swap(type_names_[nb_marker_attribs_]);
		}
		nb_marker_attribs_++;

		return ptr;
	}

	/**
	 * @brief remove an attribute by its name
	 * @param attribute_name name of attribute to remove
	 * @return true if attribute exists and has been removed
	 */
	bool removeAttribute(const std::string& attribute_name)
	{
		unsigned int index = getArrayIndex(attribute_name);

		if (index == UNKNOWN)
		{
			std::cerr << "removeAttribute by name: attribute not found (" << attribute_name << ")" << std::endl;
			return false;
		}

		removeAttribute(index);

		return true;
	}

	/**
	 * @brief remove an attribute by its ChunkArray pointer
	 * @param ptr ChunkArray pointer to the attribute to remove
	 * @return true if attribute exists and has been removed
	 */
	bool removeAttribute(const ChunkArrayGen<CHUNKSIZE>* ptr)
	{
		unsigned int index = getArrayIndex(ptr);

		if (index == UNKNOWN)
		{
			std::cerr << "removeAttribute by ptr: attribute not found" << std::endl;
			return false;
		}

		removeAttribute(index);

		return true;
	}

	/**
	 * @brief Number of attributes of the container
	 * @return number of attributes
	 */
	unsigned int getNbAttributes() const
	{
		return table_arrays_.size();
	}

	/**
	* @brief get a chunk_array
	* @param attribName name of the array
	* @return pointer on typed chunk_array
	*/
	template<typename T>
	ChunkArray<CHUNKSIZE,T>* getDataArray(const std::string& attribute_name)
	{
		unsigned int index = getArrayIndex(attribute_name);
		if(index == UNKNOWN)
			return nullptr;

		ChunkArray<CHUNKSIZE,T>* atm = dynamic_cast<ChunkArray<CHUNKSIZE, T>*>(table_arrays_[index]);

		cgogn_message_assert(atm != nullptr, "getDataArray: wrong type");

		return atm;
	}

	/**
	* @brief get a chunk_array
	* @param attribName name of the array
	* @return pointer on virtual chunk_array
	*/
	ChunkArrayGen<CHUNKSIZE>* getVirtualDataArray(const std::string& attribute_name)
	{
		unsigned int index = getArrayIndex(attribute_name);
		if(index == UNKNOWN)
			return nullptr;

		return table_arrays_[index];
	}

	/**
	 * @brief size (number of used lines)
	 * @return the number of lines
	 */
	unsigned int size() const
	{
		return nb_used_lines_;
	}

	/**
	 * @brief capacity (number of reserved lines)
	 * @return number of reserved lines
	 */
	unsigned int capacity() const
	{
		return refs_.capacity();
	}

	/**
	 * @brief setCurrentBrowser
	 * @param browser, pointer to a heap-allocated ContainerBrowser
	 */
	inline void setCurrentBrowser(ContainerBrowser* browser)
	{
		if (current_browser_ != std_browser_.get())
			delete current_browser_;
		current_browser_ = browser;
	}

	inline void setStandardBrowser()
	{
		if (current_browser_ != std_browser_.get())
			delete current_browser_;
		current_browser_ = std_browser_;
	}

	/**
	 * @brief begin
	 * @return the index of the first used line of the container
	 */
	inline unsigned int begin() const
	{
		return current_browser_->begin();
	}

	/**
	 * @brief end
	 * @return the index after the last used line of the container
	 */
	inline unsigned int end() const
	{
		return current_browser_->end();
	}

	/**
	 * @brief next it <- next used index in the container
	 * @param it index to "increment"
	 */
	inline void next(unsigned int& it) const
	{
		current_browser_->next(it);
	}

	/**
	 * @brief next primitive: it <- next primitive used index in the container (eq to PRIMSIZE next)
	 * @param it index to "increment"
	 */
	inline void nextPrimitive(unsigned int& it, unsigned int prim_size) const
	{
		current_browser_->nextPrimitive(it, prim_size);
	}

	/**
	 * @brief begin of container without browser
	 * @return the real index of the first used line of the container
	 */
	inline unsigned int realBegin() const
	{
		unsigned int it = 0u;
		while ((it < nb_max_lines_) && (!used(it)))
			++it;
		return it;
	}

	/**
	 * @brief end of container without browser
	 * @return the real index after the last used line of the container
	 */
	inline unsigned int realEnd() const
	{
		return nb_max_lines_;
	}

	/**
	 * @brief next without browser
	 * @param it
	 */
	inline void realNext(unsigned int& it) const
	{
		do
		{
			++it;
		} while ((it < nb_max_lines_) && (!used(it)));
	}

	/**
	 * @brief next primitive without browser
	 * @param it
	 */
	inline void realNextPrimitive(unsigned int &it, unsigned int prim_size) const
	{
		do
		{
			it += prim_size;
		} while ((it < nb_max_lines_) && (!used(it)));
	}

	/**
	 * @brief reverse begin of container without browser
	 * @return the real index of the first used line of the container in reverse order
	 */
	unsigned int realRBegin() const
	{
		unsigned int it = nb_max_lines_- 1u;
		while ((it != 0xffffffff) && (!used(it)))
			--it;
		return it;
	}

	/**
	 * @brief reverse end of container without browser
	 * @return the real index before the last used line of the container in reverse order
	 */
	unsigned int realREnd() const
	{
		return 0xffffffff; // -1
	}

	/**
	 * @brief reverse next without browser
	 * @param it
	 */
	void realRNext(unsigned int &it) const
	{
		do
		{
			--it;
		} while ((it != 0xffffffff) && (!used(it)));
	}

	/**
	 * @brief clear the container
	 * @param remove_attributes remove the attributes (not only their data)
	 */
	void clear(bool remove_attributes = false)
	{
		nb_used_lines_ = 0u;
		nb_max_lines_ = 0u;

		// clear CA of refs
		refs_.clear();

		// clear holes
		holes_stack_.clear();

		// clear data
		for (auto arr : table_arrays_)
			arr->clear();

		// remove CA ?
		if (remove_attributes)
		{
			for (auto arr : table_arrays_)
				delete arr;
			table_arrays_.clear();
		}
	}

	/**
	 * @brief fragmentation of container (size/index of last lines): 100% = no holes
	 * @return 1 is full filled - 0 is lots of holes
	 */
	float fragmentation() const
	{
		return float(size()) / float(realEnd());
	}

	/**
	 * @brief container compacting
	 * @param map_old_new vector that contains a map from old indices to new indices (holes -> 0xffffffff)
	 */
	template <unsigned int PRIMSIZE>
	void compact(std::vector<unsigned int>& map_old_new)
	{
		map_old_new.clear();
		map_old_new.resize(realEnd(), 0xffffffff);

		unsigned int up = realRBegin();
		unsigned int down = 0u;

		while (down < up)
		{
			if (!used(down))
			{
				for(unsigned int i = 0u; i < PRIMSIZE; ++i)
				{
					unsigned rdown = down + PRIMSIZE-1u - i;
					map_old_new[up] = rdown;
					copyLine(rdown, up);
					realRNext(up);
				}
				down += PRIMSIZE;
			}
			else
				down++;
		}

		nb_max_lines_ = nb_used_lines_;

		// free unused memory blocks
		unsigned int new_nb_blocks = nb_max_lines_/CHUNKSIZE + 1u;
		for (auto arr : table_arrays_)
			arr->setNbChunks(new_nb_blocks);
		refs_.setNbChunks(new_nb_blocks);

		// clear holes
		holes_stack_.clear();
	}

	/**************************************
	 *          LINES MANAGEMENT          *
	 **************************************/

	/**
	* @brief is a line used
	* @param index index of line
	* @return true if used
	*/
	bool used(unsigned int index) const
	{
		return refs_[index] != 0;
	}

	/**
	* @brief insert a group of PRIMSIZE consecutive lines in the container
	* @return index of the first line of group
	*/
	template <unsigned int PRIMSIZE>
	unsigned int insertLines()
	{
		unsigned int index;

		if (holes_stack_.empty()) // no holes -> insert at the end
		{
			index = nb_max_lines_;
			nb_max_lines_ += PRIMSIZE;

			if (nb_max_lines_%CHUNKSIZE <= PRIMSIZE) // prim on next block ? -> add block to C.A.
			{
				for (auto arr: table_arrays_)
					arr->addChunk();
				refs_.addChunk();
			}
		}
		else
		{
			index = holes_stack_.head();
			holes_stack_.pop();
		}

		// mark lines as used
		for(unsigned int i = 0u; i < PRIMSIZE; ++i)
			refs_.setValue(index+i, 1u); // do not use [] in case of refs_ is bool

		nb_used_lines_ += PRIMSIZE;

		return index;
	}

	/**
	* @brief remove a group of PRIMSIZE lines in the container
	* @param index index of one line of group to remove
	*/
	template <unsigned int PRIMSIZE>
	void removeLines(unsigned int index)
	{
		unsigned int begin_prim_idx = (index/PRIMSIZE) * PRIMSIZE;

		cgogn_message_assert(this->used(begin_prim_idx), "Error removing non existing index");

		holes_stack_.push(begin_prim_idx);

		// mark lines as unused
		for(unsigned int i = 0u; i < PRIMSIZE; ++i)
			refs_.setValue(begin_prim_idx++, 0u); // do not use [] in case of refs_ is bool

		nb_used_lines_ -= PRIMSIZE;
	}

	/**
	 * @brief initialize a line of the container (an element of each attribute)
	 * @param index line index
	 */
	void initLine(unsigned int index)
	{
		cgogn_message_assert(!used(index), "initLine only with allocated lines");

		for (auto ptrAtt : table_arrays_)
			// if (ptrAtt != nullptr) never null !
			ptrAtt->initElement(index);
	}

	void initMarkersOfLine(unsigned int index)
	{
		cgogn_message_assert(!used(index), "initMarkersOfLine only with allocated lines");

		for (unsigned int i = 0u; i < nb_marker_attribs_; ++i)
			table_arrays_[i]->initElement(index);
	}

	/**
	 * @brief copy the content of line src in line dst (with refs)
	 * @param dstIndex destination
	 * @param srcIndex source
	 */
	void copyLine(unsigned int dst, unsigned int src)
	{
		for (auto ptrAtt : table_arrays_)
			if (ptrAtt != nullptr)
				ptrAtt->copyElement(dst, src);
		refs_[dst] = refs_[src];
	}

	/**
	* @brief increment the reference counter of the given line (only for PRIMSIZE==1)
	* @param index index of the line
	*/
	void refLine(unsigned int index)
	{
		// static_assert(PRIMSIZE == 1u, "refLine with container where PRIMSIZE!=1");
		refs_[index]++;
	}

	/**
	* @brief decrement the reference counter of the given line (only for PRIMSIZE==1)
	* @param index index of the line
	* @return true if the line was removed
	*/
	bool unrefLine(unsigned int index)
	{
		// static_assert(PRIMSIZE == 1u, "unrefLine with container where PRIMSIZE!=1");
		refs_[index]--;
		if (refs_[index] == 1u)
		{
			holes_stack_.push(index);
			refs_[index] = 0u;
			--nb_used_lines_;
			return true;
		}
		return false;
	}

	/**
	* @brief get the number of references of the given line
	* @param index index of the line
	* @return number of references of the line
	*/
	T_REF getNbRefs(unsigned int index) const
	{
		// static_assert(PRIMSIZE == 1u, "getNbRefs with container where PRIMSIZE!=1");
		return refs_[index];
	}

	void save(std::ofstream& fs)
	{
		// save info (size+used_lines+max_lines+sizeof names)
		std::vector<unsigned int> buffer;
		buffer.reserve(1024);
		buffer.push_back(static_cast<unsigned int>(table_arrays_.size()));
		buffer.push_back(nb_used_lines_);
		buffer.push_back(nb_max_lines_);
		buffer.push_back(nb_marker_attribs_);
		for(unsigned int i = 0u; i < table_arrays_.size(); ++i)
		{
			buffer.push_back(static_cast<unsigned int>(names_[i].size()+1));
			buffer.push_back(static_cast<unsigned int>(type_names_[i].size()+1));
		}
		fs.write(reinterpret_cast<const char*>(&(buffer[0])),std::streamsize(buffer.size()*sizeof(unsigned int)));

		// save names
		for(unsigned int i = 0; i < table_arrays_.size(); ++i)
		{
			const char* s1 = names_[i].c_str();
			const char* s2 = type_names_[i].c_str();
			fs.write(s1, std::streamsize((names_[i].size()+1u)*sizeof(char)));
			fs.write(s2, std::streamsize((type_names_[i].size()+1u)*sizeof(char)));
		}

		// save chunk arrays
		for(unsigned int i = 0u; i < table_arrays_.size(); ++i)
		{
			table_arrays_[i]->save(fs, nb_max_lines_);
		}
		// save uses/refs
		refs_.save(fs, nb_max_lines_);

		// save stack
		holes_stack_.save(fs, holes_stack_.size());
	}

	bool load(std::ifstream& fs)
	{
		// read info
		unsigned int buff1[4];
		fs.read(reinterpret_cast<char*>(buff1), 4u*sizeof(unsigned int));

		nb_used_lines_ = buff1[1];
		nb_max_lines_ = buff1[2];
		nb_marker_attribs_ = buff1[3];

		std::vector<unsigned int> buff2(2u*buff1[0]);
		fs.read(reinterpret_cast<char*>(&(buff2[0])), std::streamsize(2u*buff1[0]*sizeof(unsigned int)));

		names_.resize(buff1[0]);
		type_names_.resize(buff1[0]);

		// read name
		char buff3[256];
		for(unsigned int i = 0u; i < buff1[0]; ++i)
		{
			fs.read(buff3, std::streamsize(buff2[2u*i]*sizeof(char)));
			names_[i] = std::string(buff3);

			fs.read(buff3, std::streamsize(buff2[2u*i+1u]*sizeof(char)));
			type_names_[i] = std::string(buff3);
		}

		// read chunk array
		table_arrays_.resize(buff1[0]);
		bool ok=true;
		for (unsigned int i = 0u; i < buff1[0]; ++i)
		{
			table_arrays_[i] = ChunkArrayFactory<CHUNKSIZE>::create(type_names_[i]);
			ok &= table_arrays_[i]->load(fs);
		}
		ok &= refs_.load(fs);

		return ok;
	}
};

} // namespace cgogn

#endif // CORE_CONTAINER_CHUNK_ARRAY_CONTAINER_H_
