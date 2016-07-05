template <class T> int32_t get_vector_sum(svector<T> &vec)
{
	T total=0;
	auto ii_s=vec.begin(),ii_e=vec.end();
	for(;ii_s<ii_e;++ii_s)
		{
		total+=(*ii_s);
		}
	return total;
}

template <class T> int32_t get_random_biased_index(svector<T> &chance)
{
	if(chance.size()==0)
		{
		errorlog_string("Empty biased index vector");
		return -1;
		}

	T roll=trandom(get_vector_sum(chance));

	auto ii_s=chance.begin(),ii_e=chance.end();
	auto ii_b=ii_s;
	for(;ii_s<ii_e;++ii_s)
		{
		if(roll<(*ii_s))return (int32_t)(ii_s-ii_b);
		roll-=(*ii_s);
		}

	errorlog_string("Biased index vector computation error");
	return 0;//MORE FUNCTIONS WILL BE HAPPIER WITH 0 THAN -1 HERE
}

template <class T> void zero_vector(svector<T> &vc)
{
	//NOTE: vector MEMORY IS GUARANTEED TO BE CONTIGUOUS, AND THIS IS FASTER THAN GOING THROUGH ONE BY ONE
			//apparently this gives linux a headache though, so back to the slower way
	//int32_t sz=vc.size();
	//if(sz==0)return;
	//memset(&(vc[0]),0,sizeof(T)*sz);
	auto ii_s=vc.begin(),ii_e=vc.end();
	for(;ii_s<ii_e;++ii_s)(*ii_s)=0;
}

template <class T> bool positive_vector(svector<T> &vc)
{
	auto ii_s=vc.begin(),ii_e=vc.end();
	for(;ii_s<ii_e;++ii_s)
		{
		if((*ii_s)>0)return true;
		}
	return false;
}

template <class T> void add_unique_to_vector(T nl,svector<T> &vc)
{
	auto ii_s=vc.begin(),ii_e=vc.end();
	for(;ii_s<ii_e;++ii_s)
		{
		if((*ii_s)==nl)return;
		}
	vc.push_back(nl);
}

template <class T,class T2> void add_dual_unique_to_vectors(T nl,T2 nl2,svector<T> &vc,svector<T2> &vc2)
{
	auto ii_s=vc.begin(),ii_e=vc.end();
	auto ii_s2=vc2.begin(),ii_e2=vc2.end();
	for(;ii_s<ii_e;++ii_s,++ii_s2)
		{
		if((*ii_s)==nl&&
			(*ii_s2)==nl2)return;
		}
	vc.push_back(nl);
	vc2.push_back(nl2);
}

template <class T,class T2,class T3,class T4> void add_quad_unique_to_vectors(T nl,T2 nl2,T3 nl3,T4 nl4,
																				svector<T> &vc,svector<T2> &vc2,svector<T3> &vc3,svector<T4> &vc4)
{
	auto ii_s=vc.begin(),ii_e=vc.end();
	auto ii_s2=vc2.begin(),ii_e2=vc2.end();
	auto ii_s3=vc3.begin(),ii_e3=vc3.end();
	auto ii_s4=vc4.begin(),ii_e4=vc4.end();
	for(;ii_s<ii_e;++ii_s,++ii_s2,++ii_s3,++ii_s4)
		{
		if((*ii_s)==nl&&
			(*ii_s2)==nl2&&
			(*ii_s3)==nl3&&
			(*ii_s4)==nl4)return;
		}
	vc.push_back(nl);
	vc2.push_back(nl2);
	vc3.push_back(nl3);
	vc4.push_back(nl4);
}

template <class T> void remove_all_from_vector(T nl,svector<T> &vc)
{
	int32_t i;
	for(i=(int32_t)vc.size()-1;i>=0;i--)
		{
		if(vc[i]==nl)vc.erase(i);
		}
}

template <class T,class T2> void remove_all_from_dual_vectors(T nl,T2 nl2,svector<T> &vc,svector<T2> &vc2)
{
	int32_t i;
	for(i=(int32_t)vc.size()-1;i>=0;i--)
		{
		if(vc[i]==nl&&
			vc2[i]==nl2)
			{
			vc.erase(i);
			vc2.erase(i);
			}
		}
}

template <class T> int32_t get_vector_index(T a,svector<T> &v)
{
	auto ii_s=v.begin(),ii_e=v.end();
	auto ii_b=ii_s;
	for(;ii_s<ii_e;++ii_s)
		{
		if((*ii_s)==a)return (int32_t)(ii_s-ii_b);
		}
	return -1;
}

template <class T,class T2> int32_t get_dual_vector_index(T a1,T2 a2,svector<T> &vc,svector<T2> &vc2)
{
	auto ii_s=vc.begin(),ii_e=vc.end();
	auto ii_s2=vc2.begin(),ii_e2=vc2.end();
	auto ii_b=ii_s;
	for(;ii_s<ii_e;++ii_s,++ii_s2)
		{
		if((*ii_s)==a1&&
			(*ii_s2)==a2)return (int32_t)(ii_s-ii_b);
		}
	return -1;
}

template <class T,class T2,class T3,class T4> int32_t get_quad_vector_index(T a1,T2 a2,T3 a3,T4 a4,
																		svector<T> &vc,svector<T2> &vc2,
																		svector<T3> &vc3,svector<T4> &vc4)
{
	auto ii_s=vc.begin(),ii_e=vc.end();
	auto ii_s2=vc2.begin(),ii_e2=vc2.end();
	auto ii_s3=vc3.begin(),ii_e3=vc3.end();
	auto ii_s4=vc4.begin(),ii_e4=vc4.end();
	auto ii_b=ii_s;
	for(;ii_s<ii_e;++ii_s,++ii_s2,++ii_s3,++ii_s4)
		{
		if((*ii_s)==a1&&
			(*ii_s2)==a2&&
			(*ii_s3)==a3&&
			(*ii_s4)==a4)return (int32_t)(ii_s-ii_b);
		}
	return -1;
}

template <class T> void merge_vectors(T &master, T &merger)
{
	auto ii_s=merger.begin(),ii_e=merger.end();
	for(;ii_s<ii_e;++ii_s)
		{
		auto ii_s2=master.begin(),ii_e2=master.end();
		for(;ii_s2<ii_e2;++ii_s2)
			{
			if((*ii_s)==(*ii_s2))break;
			}
		if(ii_s2>=ii_e2)
			{
			master.push_back((*ii_s));
			}
		}
}

template <class T> int32_t get_common_element_vector_index(T &master, T &merger)
{
	auto ii_s=merger.begin(),ii_e=merger.end();
	auto ii_s2=master.begin(),ii_e2=master.end();
	auto ii_b2=ii_s2;

	for(;ii_s<ii_e;++ii_s)
		{
		for(;ii_s2<ii_e2;++ii_s2)
			{
			if((*ii_s)==(*ii_s2))return (int32_t)(ii_s2-ii_b2);
			}
		ii_s2=ii_b2;
		}

	return -1;
}

template <class T,class T2> void merge_dual_vectors(T &master, T2 &master2, T &merger, T2 &merger2)
{
	auto ii_s=merger.begin(),ii_e=merger.end();
	auto ii_s2=merger2.begin(),ii_e2=merger2.end();
	for(;ii_s<ii_e;++ii_s,++ii_s2)
		{
		auto ii_s3=master.begin(),ii_e3=master.end();
		auto ii_s4=master2.begin(),ii_e4=master2.end();
		for(;ii_s3<ii_e3;++ii_s3,++ii_s4)
			{
			if((*ii_s)==(*ii_s3)&&
				(*ii_s2)==(*ii_s4))break;
			}
		if(ii_s3>=ii_e3)
			{
			master.push_back((*ii_s));
			master2.push_back((*ii_s2));
			}
		}
}

template <class T,class T2,class T3,class T4> void merge_quad_vectors(T &master, T2 &master2, T3 &master3, T4 &master4,
																		T &merger, T2 &merger2, T3 &merger3, T4 &merger4)
{
	auto ii_s=merger.begin(),ii_e=merger.end();
	auto ii_s2=merger2.begin(),ii_e2=merger2.end();
	auto ii_s3=merger3.begin(),ii_e3=merger3.end();
	auto ii_s4=merger4.begin(),ii_e4=merger4.end();
	for(;ii_s<ii_e;++ii_s,++ii_s2,++ii_s3,++ii_s4)
		{
		auto ii_s5=master.begin(),ii_e5=master.end();
		auto ii_s6=master2.begin(),ii_e6=master2.end();
		auto ii_s7=master3.begin(),ii_e7=master3.end();
		auto ii_s8=master4.begin(),ii_e8=master4.end();
		for(;ii_s5<ii_e5;++ii_s5,++ii_s6,++ii_s7,++ii_s8)
			{
			if((*ii_s)==(*ii_s5)&&
				(*ii_s2)==(*ii_s6)&&
				(*ii_s3)==(*ii_s7)&&
				(*ii_s4)==(*ii_s8))break;
			}
		if(ii_s5>=ii_e5)
			{
			master.push_back((*ii_s));
			master2.push_back((*ii_s2));
			master3.push_back((*ii_s3));
			master4.push_back((*ii_s4));
			}
		}
}

template <class T> void cull_vectors(T &master,T &cull)
{
	int32_t i,i2;
	for(i=(int32_t)cull.size()-1;i>=0;i--)
		{
		for(i2=(int32_t)master.size()-1;i2>=0;i2--)
			{
			if(cull[i]==master[i2])
				{
				master.erase(i2);
				break;
				}
			}
		}
}

template <class T> void push_on_vector(T &master,T &new_stuff)
{
	auto ii_s=new_stuff.begin(),ii_e=new_stuff.end();
	for(;ii_s<ii_e;++ii_s)
		{
		master.push_back(*ii_s);
		}
}

template<class T> VIndex add_to_global_id_vector(T ptr,svector<T> &vect)
{
	int32_t size=(int32_t)vect.size();
	if(size==0)
		{
		vect.push_back(ptr);
		return 1;
		}
	if(vect[size-1]->global_id<ptr->global_id)
		{
		vect.push_back(ptr);
		return size;
		}

	int32_t start=0;
	int32_t end=size-1;
	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->global_id==ptr->global_id)return -1;
		if(start==end)
			{
			if(cptr->global_id<ptr->global_id)
				{
				if(start+1>=size)return -1;//push_back() FOR UNIQUE ALREADY HANDLED
				else
					{
					vect.insert(start+1,ptr);
					return start+1;
					}
				}
			else if(cptr->global_id>ptr->global_id)
				{
				vect.insert(start,ptr);
				return start;
				}
			return -1;
			}

		if(cptr->global_id>ptr->global_id)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr->global_id<ptr->global_id)
			{
			if(start+1>=size)return -1;//push_back() FOR UNIQUE ALREADY HANDLED
			else
				{
				vect.insert(start+1,ptr);
				return start+1;
				}
			}
		else if(cptr->global_id>ptr->global_id)
			{
			vect.insert(start,ptr);
			return start;
			}
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr->global_id<ptr->global_id)
			{
			if(end+1>=size)return -1;//push_back() FOR UNIQUE ALREADY HANDLED
			else
				{
				vect.insert(end+1,ptr);
				return end+1;
				}
			}
		else if(cptr->global_id>ptr->global_id)
			{
			vect.insert(end,ptr);
			return end;
			}
		}
	return -1;//push_back() FOR UNIQUE ALREADY HANDLED SO NO else
}

template<class T> void remove_from_global_id_vector(T ptr,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;
		cptr=vect[mid];
		if(cptr==ptr)
			{
			vect.erase(mid);
			return;
			}

		if(cptr->global_id>ptr->global_id)end=mid-1;
		else start=mid+1;
		}
}

template<class T> void remove_from_global_id_vector_by_id(int32_t id,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;
		cptr=vect[mid];
		if(cptr->global_id==id)
			{
			vect.erase(mid);
			return;
			}

		if(cptr->global_id>id)end=mid-1;
		else start=mid+1;
		}
}

template<class T> T get_from_global_id_vector(int32_t id,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->global_id==id)return cptr;
		else if(cptr->global_id>id)end=mid-1;
		else start=mid+1;
		}

	return NULL;
}

template<class T> int32_t get_index_from_global_id_vector(int32_t id,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->global_id==id)return mid;
		else if(cptr->global_id>id)end=mid-1;
		else start=mid+1;
		}

	return -1;
}

template<class T> VIndex add_to_binary_vector(T ptr,svector<T> &vect)
{
	int32_t size=vect.size();
	if(size==0)
		{
		vect.push_back(ptr);
		return 0;
		}
	if(vect[size-1]<ptr)
		{
		vect.push_back(ptr);
		return size;
		}

	int32_t start=0;
	int32_t end=size-1;
	int32_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)
			{
			vect.insert(mid,ptr);//INSERT A COPY HERE SINCE THIS IS A NON-UNIQUE VECTOR
			return mid;
			}
		if(start==end)
			{
			if(cptr<ptr)
				{
				if(start+1>=size)vect.push_back(ptr);
				else vect.insert(start+1,ptr);
				return start+1;
				}
			else
				{
				vect.insert(start,ptr);
				return start;
				}
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr<ptr)
			{
			if(start+1>=size)vect.push_back(ptr);
			else vect.insert(start+1,ptr);
			return start+1;
			}
		else
			{
			vect.insert(start,ptr);
			return start;
			}
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr<ptr)
			{
			if(end+1>=size)vect.push_back(ptr);
			else vect.insert(end+1,ptr);
			return end+1;
			}
		else
			{
			vect.insert(end,ptr);
			return end;
			}
		}
	else vect.push_back(ptr);
	return end;
}

//NOTE: RETURNS -1 IF ALREADY PRESENT, NOT THE INDEX
template<class T> VIndex add_unique_to_binary_vector(T ptr,svector<T> &vect)
{
	int32_t size=(int32_t)vect.size();
	if(size==0)
		{
		vect.push_back(ptr);
		return 0;
		}
	if(vect[size-1]<ptr)
		{
		vect.push_back(ptr);
		return size;
		}

	int32_t start=0;
	int32_t end=size-1;
	int32_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)return -1;//ALREADY IN VECTOR
		if(start==end)
			{
			if(cptr<ptr)
				{
				if(start+1>=(int32_t)vect.size())return -1;//push_back() CASE ALREADY HANDLED

				vect.insert(start+1,ptr);
				return start+1;
				}
			else if(cptr>ptr)
				{
				vect.insert(start,ptr);
				return start;
				}
			return -1;
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr<ptr)
			{
			if(start+1>=size)return -1;//push_back() CASE ALREADY HANDLED

			vect.insert(start+1,ptr);
			return start+1;
			}
		else if(cptr>ptr)
			{
			vect.insert(start,ptr);
			return start;
			}
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr<ptr)
			{
			if(end+1>=size)return -1;//push_back() CASE ALREADY HANDLED

			vect.insert(end+1,ptr);
			return end+1;
			}
		else if(cptr>ptr)
			{
			vect.insert(end,ptr);
			return end;
			}
		}
	return -1;//push_back() CASE ALREADY HANDLED SO NO else
}

//NOTE: RETURNS INDEX IF ALREADY PRESENT
template<class T> VIndex add_unique_to_binary_vector_always_index(T ptr,svector<T> &vect,bool &was_present)
{
	was_present=false;

	int32_t size=(int32_t)vect.size();
	if(size==0)
		{
		vect.push_back(ptr);
		return 0;
		}
	if(vect[size-1]<ptr)
		{
		vect.push_back(ptr);
		return size;
		}

	int32_t start=0;
	int32_t end=size-1;
	int32_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr){was_present=true;return mid;}//ALREADY IN VECTOR
		if(start==end)
			{
			if(cptr<ptr)
				{
				if(start+1>=(int32_t)vect.size())return -1;//push_back() CASE ALREADY HANDLED

				vect.insert(start+1,ptr);
				return start+1;
				}
			else if(cptr>ptr)
				{
				vect.insert(start,ptr);
				return start;
				}
			return -1;
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr<ptr)
			{
			if(start+1>=size)return -1;//push_back() CASE ALREADY HANDLED

			vect.insert(start+1,ptr);
			return start+1;
			}
		else if(cptr>ptr)
			{
			vect.insert(start,ptr);
			return start;
			}
		if(cptr==ptr){was_present=true;return start;}//ALREADY IN VECTOR
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr<ptr)
			{
			if(end+1>=size)return -1;//push_back() CASE ALREADY HANDLED

			vect.insert(end+1,ptr);
			return end+1;
			}
		else if(cptr>ptr)
			{
			vect.insert(end,ptr);
			return end;
			}
		if(cptr==ptr){was_present=true;return end;}//ALREADY IN VECTOR
		}
	return -1;//push_back() CASE ALREADY HANDLED SO NO else
}

template<class T> void remove_from_binary_vector(T ptr,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;
	int32_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)
			{
			vect.erase(mid);
			return;
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}
}

template<class T> int32_t get_index_from_binary_vector(T id,svector<T> &vect)
{
	int32_t size=(int32_t)vect.size();
	if(size==0||id==-1)return -1;

	int32_t start=0;
	int32_t end=size-1;
	int32_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==id)
			{
			return mid;
			}

		if(cptr>id)end=mid-1;
		else start=mid+1;
		}

	return -1;
}

template<class T> int32_t get_floor_index_from_binary_vector(T ptr,svector<T> &vect)
{
	int32_t size=(int32_t)vect.size();
	if(size==0)return -1;
	if(vect[size-1]<ptr)return size-1;

	int32_t start=0;
	int32_t end=size-1;
	int32_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)
			{
			while(mid>0)
				{
				if(vect[mid-1]<ptr)return mid;
				mid--;
				}
			return mid;
			}
		if(start==end)
			{
			if(cptr<ptr)
				{
				if(start+1>=size)return start;
				else return start;
				}
			//NOTE: cptr>ptr as == is already handled above
			else
				{
				if(start<=0)return 0;
				else return start-1;
				}
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr<ptr)
			{
			if(start+1>=size)return start;
			else return start;
			}
		else if(cptr>ptr)
			{
			if(start<=0)return 0;
			else return start-1;
			}
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr<ptr)
			{
			if(end+1>=size)return end;
			else return end;
			}
		else if(cptr>ptr)
			{
			if(end<=0)return 0;
			else return end-1;
			}
		}
	return start;
}

template<class T> void fixed_array_push_back(T ptr,T *vect,int32_t &size,int32_t max)
{
	if(size>=max)return;
	vect[size]=ptr;
	size++;
}

template<class T> void fixed_array_insert(int32_t index,T ptr,T *vect,int32_t &size,int32_t max)
{
	if(size>=max)return;
	if(size>0)
		{
		T *ptr1=vect+size;
		T *ptr2=vect+(size-1);
		T *stop=vect+index;
		while(ptr2>=stop)
			{
			(*ptr1)=(*ptr2);
			--ptr1;
			--ptr2;
			}
		(*ptr1)=ptr;
		return;
		}
	else
		{
		vect[index]=ptr;
		++size;
		}
}

template<class T> void add_to_fixed_binary_array(T ptr,T *vect,int32_t &size,int32_t max)
{
	if(size==0)
		{
		fixed_array_push_back(ptr,vect,size,max);
		return;
		}
	if(vect[size-1]<ptr)
		{
		fixed_array_push_back(ptr,vect,size,max);
		return;
		}

	int32_t start=0;
	int32_t end=size-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)
			{
			fixed_array_insert(mid,ptr,vect,size,max);//INSERT A COPY SINCE THIS IS NON-UNIQUE CASE
			return;
			}
		if(start==end)
			{
			if(cptr<ptr)
				{
				if(start+1>=size)fixed_array_push_back(ptr,vect,size,max);
				else fixed_array_insert(start+1,ptr,vect,size,max);
				}
			else fixed_array_insert(start,ptr,vect,size,max);
			return;
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr<ptr)
			{
			if(start+1>=size)fixed_array_push_back(ptr,vect,size,max);
			else fixed_array_insert(start+1,ptr,vect,size,max);
			}
		else fixed_array_insert(start,ptr,vect,size,max);
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr<ptr)
			{
			if(end+1>=size)fixed_array_push_back(ptr,vect,size,max);
			else fixed_array_insert(end+1,ptr,vect,size,max);
			}
		else fixed_array_insert(end,ptr,vect,size,max);
		}
	else fixed_array_push_back(ptr,vect,size,max);
}

template<class T> void add_unique_to_fixed_binary_array(T ptr,T *vect,int32_t &size,int32_t max)
{
	if(size==0)
		{
		fixed_array_push_back(ptr,vect,size,max);
		return;
		}
	if(vect[size-1]<ptr)
		{
		fixed_array_push_back(ptr,vect,size,max);
		return;
		}

	int32_t start=0;
	int32_t end=size-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)return;
		if(start==end)
			{
			if(cptr<ptr)
				{
				if(start+1>=size)return;//WAS push_back()
				else fixed_array_insert(start+1,ptr,vect,size,max);
				}
			else if(cptr>ptr)fixed_array_insert(start,ptr,vect,size,max);
			return;
			}

		if(cptr>ptr)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr<ptr)
			{
			if(start+1>=size)return;//WAS push_back()
			else fixed_array_insert(start+1,ptr,vect,size,max);
			}
		else if(cptr>ptr)fixed_array_insert(start,ptr,vect,size,max);
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr<ptr)
			{
			if(end+1>=size)return;//WAS push_back()
			else fixed_array_insert(end+1,ptr,vect,size,max);
			}
		else if(cptr>ptr)fixed_array_insert(end,ptr,vect,size,max);
		}
	//NOTE: NO else CASE HERE BECAUSE IN THE NON-UNIQUE VERSION IT JUST PUSHED BACK
		//AND PUSHING BACK IS HANDLED IN THE OPENING FUNCTION
}

template<class T> void add_to_local_id_vector(T ptr,svector<T> &vect)
{
	int32_t size=(int32_t)vect.size();
	if(size==0)
		{
		vect.push_back(ptr);
		return;
		}
	if(vect[size-1]->local_id<ptr->local_id)
		{
		vect.push_back(ptr);
		return;
		}

	int32_t start=0;
	int32_t end=size-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->local_id==ptr->local_id)return;
		if(start==end)
			{
			if(cptr->local_id<ptr->local_id)
				{
				if(start+1>=size)return;//push_back() FOR UNIQUE ALREADY HANDLED
				else vect.insert(start+1,ptr);
				}
			else if(cptr->local_id>ptr->local_id)vect.insert(start,ptr);
			return;
			}

		if(cptr->local_id>ptr->local_id)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr->local_id<ptr->local_id)
			{
			if(start+1>=size)return;//push_back() FOR UNIQUE ALREADY HANDLED
			else vect.insert(start+1,ptr);
			}
		else if(cptr->local_id>ptr->local_id)vect.insert(start,ptr);
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr->local_id<ptr->local_id)
			{
			if(end+1>=size)return;//push_back() FOR UNIQUE ALREADY HANDLED
			else vect.insert(end+1,ptr);
			}
		else if(cptr->local_id>ptr->local_id)vect.insert(end,ptr);
		}
}

template<class T> void remove_from_local_id_vector(T ptr,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr==ptr)
			{
			vect.erase(mid);
			return;
			}

		if(cptr->local_id>ptr->local_id)end=mid-1;
		else start=mid+1;
		}
}

template<class T> T get_from_local_id_vector(int32_t id,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->local_id==id)return cptr;
		else if(cptr->local_id>id)end=mid-1;
		else start=mid+1;
		}

	return NULL;
}

template<class T> int32_t get_index_from_local_id_vector(int32_t id,svector<T> &vect)
{
	int32_t start=0;
	int32_t end=(int32_t)vect.size()-1;

	T cptr;
	int32_t mid;
	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->local_id==id)return mid;
		else if(cptr->local_id>id)end=mid-1;
		else start=mid+1;
		}

	return -1;
}

template<class T> void add_to_short_id_vector(T ptr,svector<T> &vect)
{
	int16_t size=(int16_t)vect.size();
	if(size==0)
		{
		vect.push_back(ptr);
		return;
		}
	if(vect[size-1]->short_id<ptr->short_id)
		{
		vect.push_back(ptr);
		return;
		}

	int16_t start=0;
	int16_t end=size-1;
	int16_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->short_id==ptr->short_id)return;
		if(start==end)
			{
			if(cptr->short_id<ptr->short_id)
				{
				if(start+1>=size)return;//push_back() FOR UNIQUE ALREADY HANDLED
				else vect.insert(start+1,ptr);
				}
			else if(cptr->short_id>ptr->short_id)vect.insert(start,ptr);
			return;
			}

		if(cptr->short_id>ptr->short_id)end=mid-1;
		else start=mid+1;
		}

	if(end<0)
		{
		T cptr=vect[start];
		if(cptr->short_id<ptr->short_id)
			{
			if(start+1>=size)return;//push_back() FOR UNIQUE ALREADY HANDLED
			else vect.insert(start+1,ptr);
			}
		else if(cptr->short_id>ptr->short_id)vect.insert(start,ptr);
		}
	else if(end<size)
		{
		T cptr=vect[end];
		if(cptr->short_id<ptr->short_id)
			{
			if(end+1>=size)return;//push_back() FOR UNIQUE ALREADY HANDLED
			else vect.insert(end+1,ptr);
			}
		else if(cptr->short_id>ptr->short_id)vect.insert(end,ptr);
		}
}

template<class T> void remove_from_short_id_vector(T ptr,svector<T> &vect)
{
	int16_t start=0;
	int16_t end=(int16_t)vect.size()-1;
	T cptr;

	while(start<=end)
		{
		int16_t mid=(start+end)>>1;

		T cptr=vect[mid];
		if(cptr==ptr)
			{
			vect.erase(mid);
			return;
			}

		if(cptr->short_id>ptr->short_id)end=mid-1;
		else start=mid+1;
		}
}

template<class T> T get_from_short_id_vector(int16_t id,svector<T> &vect)
{
	int16_t start=0;
	int16_t end=(int16_t)vect.size()-1;
	int16_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->short_id==id)
			{
			return cptr;
			}

		if(cptr->short_id>id)end=mid-1;
		else start=mid+1;
		}

	return NULL;
}

template<class T> int16_t get_index_from_short_id_vector(int16_t id,svector<T> &vect)
{
	int16_t start=0;
	int16_t end=(int16_t)vect.size()-1;
	int16_t mid;
	T cptr;

	while(start<=end)
		{
		mid=(start+end)>>1;

		cptr=vect[mid];
		if(cptr->short_id==id)
			{
			return mid;
			}

		if(cptr->short_id>id)end=mid-1;
		else start=mid+1;
		}

	return -1;
}