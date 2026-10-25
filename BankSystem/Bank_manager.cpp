#include "Bank_manager.h"

#include <climits>
#include <cstring>

Customer::Customer()
	: account_number(0), name(nullptr), balance(0)
{
	copy_name("");
}

Customer::Customer(int account_number, const char* name, int balance)
	: account_number(account_number), name(nullptr), balance(balance)
{
	copy_name(name);
}

Customer::Customer(const Customer& other)
	: account_number(other.account_number), name(nullptr), balance(other.balance)
{
	copy_name(other.name);
}

Customer& Customer::operator=(const Customer& other)
{
	if (this == &other)
		return *this;

	account_number = other.account_number;
	balance = other.balance;

	delete[] name;
	name = nullptr;
	copy_name(other.name);
	return *this;
}

Customer::~Customer()
{
	delete[] name;
	name = nullptr;
}

void Customer::copy_name(const char* src)
{
	if (src == nullptr)
		src = "";

	size_t length = strlen(src);
	name = new char[length + 1];

	for (size_t i = 0; i < length; i++)
		name[i] = src[i];

	name[length] = '\0';
}

bool Customer::deposit(int amount)
{
	if (amount <= 0)
		return false;

	if (balance > INT_MAX - amount)
		return false;

	balance += amount;
	return true;
}

bool Customer::withdraw(int amount)
{
	if (amount <= 0 || amount > balance)
		return false;

	balance -= amount;
	return true;
}

Cache_data::Cache_data()
	: count(0), file_name("")
{
	for (int i = 0; i < MAX_CUSTOMERS; i++)
	{
		cache_account_data[i].pos = 0;
		cache_account_data[i].account_number = 0;
		cache_account_data[i].name_length = 0;
		cache_account_data[i].balance = 0;
	}
}

int Cache_data::find_by_key(int account_number) const
{
	for (int i = 0; i < count; i++)
	{
		if (cache_account_data[i].account_number == account_number)
			return i;
	}

	return -1;
}

bool Cache_data::add(const Cache_account_data& data)
{
	if (count >= MAX_CUSTOMERS)
		return false;

	cache_account_data[count] = data;
	count++;
	return true;
}

void Cache_data::update_balance(int index, int balance)
{
	if (index < 0 || index >= count)
		return;

	cache_account_data[index].balance = balance;
}

void Cache_data::clear()
{
	count = 0;
}

Bank_manager::Bank_manager(string bank_name)
	: file_name(""), fw(), fr(), account_count(0), cache()
{
	set_file_name(bank_name);
	setup_file();
	load_cache();
}

Bank_manager::~Bank_manager()
{
	if (fw.is_open())
		fw.close();

	if (fr.is_open())
		fr.close();
}

void Bank_manager::set_file_name(string bank_name)
{
	file_name = bank_name + ".bin";
	cache.file_name = file_name;
}

void Bank_manager::setup_file()
{
	fr.clear();
	fr.open(file_name, ios::binary);

	if (fr.is_open())
	{
		fr.close();
		return;
	}

	fr.clear();
	fr.close();

	fw.clear();
	fw.open(file_name, ios::binary | ios::out);
	fw.close();
}

streamoff Bank_manager::end_pos() const
{
	if (account_count <= 0)
		return 0;

	const Cache_account_data& last = cache.cache_account_data[account_count - 1];
	return static_cast<streamoff>(last.pos)
		+ static_cast<streamoff>(sizeof(int)) * 3
		+ static_cast<streamoff>(last.name_length);
}

bool Bank_manager::read_format(Cache_account_data& cached, char*& name_out)
{
	name_out = nullptr;

	streamoff record_pos = static_cast<streamoff>(fr.tellg());
	if (record_pos < 0)
		return false;

	int account_number = 0;
	int name_length = 0;

	if (!fr.read(reinterpret_cast<char*>(&account_number), sizeof(account_number)))
		return false;

	if (!fr.read(reinterpret_cast<char*>(&name_length), sizeof(name_length)))
		return false;

	if (name_length < 0 || name_length > MAX_NAME_LENGTH)
		return false;

	name_out = new char[name_length + 1];

	if (name_length > 0)
	{
		if (!fr.read(name_out, name_length))
		{
			delete[] name_out;
			name_out = nullptr;
			return false;
		}
	}

	name_out[name_length] = '\0';

	int balance = 0;
	if (!fr.read(reinterpret_cast<char*>(&balance), sizeof(balance)))
	{
		delete[] name_out;
		name_out = nullptr;
		return false;
	}

	cached.pos = static_cast<size_t>(record_pos);
	cached.account_number = account_number;
	cached.name_length = name_length;
	cached.balance = balance;
	return true;
}

bool Bank_manager::write_format(const Customer& customer, Cache_account_data& cached)
{
	const char* name = customer.get_name();
	if (name == nullptr)
		return false;

	int name_length = static_cast<int>(strlen(name));
	if (name_length > MAX_NAME_LENGTH)
		return false;

	fw.clear();
	fw.open(file_name, ios::binary | ios::in | ios::out);
	if (!fw.is_open())
		return false;

	streamoff record_pos = end_pos(); //streamoff는 fstream의 파일 내 바이트 위치를 나타내는 타입이고 end_pos로 파일의 끝 위치를 가져왔습니다.
	fw.seekp(record_pos, ios::beg);

	int account_number = customer.get_account_number();
	int balance = customer.get_balance();

	fw.write(reinterpret_cast<const char*>(&account_number), sizeof(account_number));
	fw.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));

	if (name_length > 0)
		fw.write(name, name_length);

	fw.write(reinterpret_cast<const char*>(&balance), sizeof(balance));

	bool ok = !fw.fail();
	fw.flush();
	fw.close();

	if (!ok)
		return false;

	cached.pos = static_cast<size_t>(record_pos);
	cached.account_number = account_number;
	cached.name_length = name_length;
	cached.balance = balance;
	return true;
}

void Bank_manager::load_cache()
{
	cache.clear();
	account_count = 0;

	fr.clear();
	fr.open(file_name, ios::binary);
	if (!fr.is_open())
		return;

	while (account_count < MAX_CUSTOMERS)
	{
		Cache_account_data cached;
		char* name = nullptr;

		if (!read_format(cached, name))
		{
			delete[] name;
			break;
		}

		delete[] name;

		if (!cache.add(cached))
			break;

		account_count++;
	}

	fr.close();
}

int Bank_manager::find_all(Customer* customers[], int max_count)
{
	if (customers == nullptr || max_count <= 0 || account_count == 0)
		return 0;

	fr.clear();
	fr.open(file_name, ios::binary);
	if (!fr.is_open())
		return 0;

	int limit = account_count < max_count ? account_count : max_count;
	int loaded = 0;

	for (int i = 0; i < limit; i++)
	{
		fr.clear();
		fr.seekg(static_cast<streamoff>(cache.cache_account_data[i].pos), ios::beg);

		Cache_account_data cached;
		char* name = nullptr;

		if (!read_format(cached, name))
		{
			delete[] name;
			break;
		}

		customers[loaded] = new Customer(cached.account_number, name, cached.balance);
		delete[] name;
		cache.update_balance(i, cached.balance);
		loaded++;
	}

	fr.close();
	return loaded;
}

bool Bank_manager::find_by_key(int account_number, Customer& out)
{
	int index = cache.find_by_key(account_number);
	if (index < 0)
		return false;

	fr.clear();
	fr.open(file_name, ios::binary);
	if (!fr.is_open())
		return false;

	fr.seekg(static_cast<streamoff>(cache.cache_account_data[index].pos), ios::beg);

	Cache_account_data cached;
	char* name = nullptr;
	bool ok = read_format(cached, name);
	fr.close();

	if (!ok)
	{
		delete[] name;
		return false;
	}

	Customer loaded(cached.account_number, name, cached.balance);
	delete[] name;
	out = loaded;
	cache.update_balance(index, cached.balance);
	return true;
}

bool Bank_manager::append(const Customer& customer)
{
	if (account_count >= MAX_CUSTOMERS)
		return false;

	if (customer.get_name() == nullptr)
		return false;

	if (static_cast<int>(strlen(customer.get_name())) > MAX_NAME_LENGTH)
		return false;

	Cache_account_data cached;
	if (!write_format(customer, cached))
		return false;

	if (!cache.add(cached))
		return false;

	account_count++;
	return true;
}

bool Bank_manager::update_balance(int account_number, int balance)
{
	int index = cache.find_by_key(account_number);
	if (index < 0)
		return false;

	const Cache_account_data& cached = cache.cache_account_data[index];

	// 현재 cache된 데이터의 balance 위치를 계산합니다
	streamoff balance_pos = static_cast<streamoff>(cached.pos) // stramoff는 fstream에서 파일 내 바이트 위치를 나타냅니다.
		+ static_cast<streamoff>(sizeof(int)) * 2 // account_number + name_length가 int 이므로 건너뜁니다
		+ static_cast<streamoff>(cached.name_length); 

	fw.clear();
	fw.open(file_name, ios::binary | ios::in | ios::out);
	if (!fw.is_open())
		return false;

	fw.seekp(balance_pos, ios::beg); // beg(파일의 시작 위치)에서 balance_pos 만큼 이동합니다.
	fw.write(reinterpret_cast<const char*>(&balance), sizeof(balance)); // 저번에 설명드렸던 바이트 형변환 케스트입니다

	bool ok = !fw.fail();
	fw.close();

	if (!ok)
		return false;

	cache.update_balance(index, balance);
	return true;
}

int Bank_manager::next_account_number() const
{
	int max_number = 0;

	for (int i = 0; i < cache.count; i++)
	{
		if (cache.cache_account_data[i].account_number > max_number)
			max_number = cache.cache_account_data[i].account_number;
	}

	return max_number + 1;
}
