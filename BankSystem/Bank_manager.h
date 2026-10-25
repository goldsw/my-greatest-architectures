#pragma once
#include <cstddef>
#include <fstream>
#include <string>

using namespace std;

const int MAX_CUSTOMERS = 100;
const int MAX_NAME_LENGTH = 100;

class Customer
{
private:
	int account_number;
	char* name;
	int balance;

	void copy_name(const char* src);

public:
	Customer();
	Customer(int account_number, const char* name, int balance);
	Customer(const Customer& other);
	Customer& operator=(const Customer& other);
	~Customer();

	int get_account_number() const { return account_number; }
	const char* get_name() const { return name; }
	int get_balance() const { return balance; }

	bool deposit(int amount);
	bool withdraw(int amount);
};

struct Cache_account_data
{
	// 파일의 시작 위치
	size_t pos;
	int account_number;
	int name_length;
	int balance;
};

struct Cache_data
{
	Cache_account_data cache_account_data[MAX_CUSTOMERS];
	int count;
	string file_name;

	Cache_data();

	int find_by_key(int account_number) const;
	bool add(const Cache_account_data& data);
	void update_balance(int index, int balance);
	void clear();
};

class Bank_manager
{
private:
	string file_name;
	ofstream fw;
	ifstream fr;
	int account_count;
	Cache_data cache;

	void set_file_name(string bank_name);
	void setup_file();
	void load_cache();
	streamoff end_pos() const;
	bool read_format(Cache_account_data& cached, char*& name_out);
	bool write_format(const Customer& customer, Cache_account_data& cached);

public:
	Bank_manager(string bank_name);
	~Bank_manager();

	int find_all(Customer* customers[], int max_count);
	bool find_by_key(int account_number, Customer& out);
	bool append(const Customer& customer);
	bool update_balance(int account_number, int balance);

	int next_account_number() const;
	int get_account_count() const { return account_count; }
	const string& get_file_name() const { return file_name; }
};
