#include <iostream>
#include <limits>
#include <string>

#include "Bank_manager.h"

using namespace std;

// 고객 객체 포인터 배열을 들고, 계좌개설/입금/출금/잔액조회를 실행한다.
class BankSystem
{
private:
	string bank_name;
	Bank_manager db;
	Customer* customers[MAX_CUSTOMERS];
	int customer_count;

	Customer* find_customer(int account_number);
	void open_account();
	void deposit();
	void withdraw();
	void show_all();
	bool read_int(const char* prompt, int& value);

public:
	BankSystem(const string& name, const string& file_stem);
	~BankSystem();

	void run();
};

BankSystem::BankSystem(const string& name, const string& file_stem)
	: bank_name(name), db(file_stem), customer_count(0)
{
	for (int i = 0; i < MAX_CUSTOMERS; i++)
		customers[i] = nullptr;

	customer_count = db.find_all(customers, MAX_CUSTOMERS);
}

BankSystem::~BankSystem()
{
	for (int i = 0; i < customer_count; i++)
	{
		delete customers[i];
		customers[i] = nullptr;
	}
}

Customer* BankSystem::find_customer(int account_number)
{
	for (int i = 0; i < customer_count; i++)
	{
		if (customers[i] != nullptr && customers[i]->get_account_number() == account_number)
			return customers[i];
	}

	return nullptr;
}

bool BankSystem::read_int(const char* prompt, int& value)
{
	cout << prompt;
	cin >> value;

	if (cin.fail()) // 입력 타입이 달라 저장 실패 시 에러 플래그
	{

		// 얘네 둘이 세트네
		cin.clear(); // 에러 플래그 해제
		cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 줄바꿈 문자를 포함한 모든 입력 버퍼를 비움
		//

		cout << "숫자를 입력해 주세요.\n";
		return false;
	}

	return true;
}

void BankSystem::open_account()
{
	if (customer_count >= MAX_CUSTOMERS)
	{
		cout << "고객은 최대 " << MAX_CUSTOMERS << "명까지 등록할 수 있습니다.\n";
		return;
	}

	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	cout << "고객 이름: ";
	string name;
	getline(cin, name);

	if (name.empty())
	{
		cout << "이름을 입력해 주세요.\n";
		return;
	}

	if (static_cast<int>(name.size()) > MAX_NAME_LENGTH)
	{
		cout << "이름이 너무 깁니다. " << MAX_NAME_LENGTH << "바이트 이하로 입력해 주세요.\n";
		// 한국어로 계산하여 출력하려고 했는데 EUC-KR과 UTF-8의 바이트 수가 서로 달라서
		// 바이트 수만 출력하겠습니다.
		return;
	}

	int number = db.next_account_number();
	Customer* customer = new Customer(number, name.c_str(), 0);

	if (!db.append(*customer))
	{
		delete customer;
		cout << "계좌 개설에 실패했습니다.\n";
		return;
	}

	customers[customer_count] = customer;
	customer_count++;

	cout << "계좌가 개설되었습니다. 계좌번호: " << number << ", 잔액: 0\n";
}

void BankSystem::deposit()
{
	int number = 0;
	if (!read_int("계좌번호: ", number))
		return;

	Customer* customer = find_customer(number);
	if (customer == nullptr)
	{
		cout << "해당 계좌번호가 없습니다.\n";
		return;
	}

	int amount = 0;
	if (!read_int("입금액: ", amount))
		return;

	if (!customer->deposit(amount))
	{
		cout << "입금액은 1원 이상이어야 합니다.\n";
		return;
	}

	if (!db.update_balance(number, customer->get_balance()))
	{
		customer->withdraw(amount);
		cout << "입금 내용을 파일에 저장하지 못했습니다.\n";
		return;
	}

	cout << "입금되었습니다. 현재 잔액: " << customer->get_balance() << "\n";
}

void BankSystem::withdraw()
{
	int number = 0;
	if (!read_int("계좌번호: ", number))
		return;

	Customer* customer = find_customer(number);
	if (customer == nullptr)
	{
		cout << "해당 계좌번호가 없습니다.\n";
		return;
	}

	int amount = 0;
	if (!read_int("출금액: ", amount))
		return;

	if (amount <= 0)
	{
		cout << "출금액은 1원 이상이어야 합니다.\n";
		return;
	}

	if (!customer->withdraw(amount))
	{
		cout << "잔액이 부족합니다. 현재 잔액: " << customer->get_balance() << "\n";
		return;
	}

	if (!db.update_balance(number, customer->get_balance()))
	{
		customer->deposit(amount);
		cout << "출금 내용을 파일에 저장하지 못했습니다.\n";
		return;
	}

	cout << "출금되었습니다. 현재 잔액: " << customer->get_balance() << "\n";
}

void BankSystem::show_all()
{
	if (customer_count == 0)
	{
		cout << "등록된 고객이 없습니다.\n";
		return;
	}

	cout << "전체 고객 수: " << customer_count << "\n";
	cout << "----------------------------------------\n";
	cout << "계좌번호\t고객이름\t잔액\n";
	cout << "----------------------------------------\n";

	for (int i = 0; i < customer_count; i++)
	{
		if (customers[i] == nullptr)
			continue;

		cout << customers[i]->get_account_number() << "\t\t"
			<< customers[i]->get_name() << "\t\t"
			<< customers[i]->get_balance() << "\n";
	}

	cout << "----------------------------------------\n";
}

void BankSystem::run()
{
	cout << bank_name << " 고객관리를 시작합니다.\n";
	cout << "데이터 파일: " << db.get_file_name() << "\n";

	while (true)
	{
		cout << "\n[" << bank_name << " 고객관리]\n";
		cout << "1. 계좌개설\n";
		cout << "2. 입금\n";
		cout << "3. 출금\n";
		cout << "4. 전체고객 잔액조회\n";
		cout << "0. 종료\n";
		cout << "선택: ";

		int menu = 0;
		cin >> menu;

		if (cin.eof())
			break;

		if (cin.fail())
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "메뉴 번호를 다시 입력해 주세요.\n";
			continue;
		}

		if (menu == 1)
			open_account();
		else if (menu == 2)
			deposit();
		else if (menu == 3)
			withdraw();
		else if (menu == 4)
			show_all();
		else if (menu == 0)
		{
			cout << "프로그램을 종료합니다.\n";
			break;
		}
		else
			cout << "없는 메뉴입니다.\n";
	}
}

int main()
{
	BankSystem bank("사과은행", "bank_data");
	bank.run();
	return 0;
}
