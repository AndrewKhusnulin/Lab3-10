#include<iostream>
#include<queue>
#include<string>
#include<cstdlib>

#include<boost/thread.hpp>
#include<boost/bind.hpp>
#include<boost/asio.hpp>
#include<boost/asio/ip/tcp.hpp>
#include<boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

typedef boost::shared_ptr<tcp::socket> socket_ptr;
typedef boost::shared_ptr<string> string_ptr;
typedef boost::shared_ptr< queue<string_ptr> > messageQueue_ptr;

io_service service;//  объект класса, который предоставляет программе связь с нативными объектами ввода/вывода / это общая очередь, которая принимает только функции, связанные с асинхронным вводом-выводом.
messageQueue_ptr messageQueue(new queue<string_ptr>);
tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);//endpoint использует сокеты для связи с хостом
const int inputSize = 256;
string_ptr promptCpy;

// Function Prototypes
bool isOwnMessage(string_ptr);
void displayLoop(socket_ptr);// добавляет элемент из сокета на наш messageQueue
void inboundLoop(socket_ptr, string_ptr);// Удаляет элементы из messageQueue для отображения на клиентском терминале
void writeLoop(socket_ptr, string_ptr);
string* buildPrompt();//обрабатывает отображение ввода терминала для клиентов
// End of Function Prototypes

int main(int argc, char** argv)
{
	try
	{
		boost::thread_group threads;//thread_group для обращения к многопроцессорной обработке
		socket_ptr sock(new tcp::socket(service));

		string_ptr prompt(buildPrompt());
		promptCpy = prompt;

		sock->connect(ep);

		cout << "Welcome to the ChatServer\nType \"exit\" to quit" << endl;

		threads.create_thread(boost::bind(displayLoop, sock));
		threads.create_thread(boost::bind(inboundLoop, sock, prompt));
		threads.create_thread(boost::bind(writeLoop, sock, prompt));

		threads.join_all();
	}
	catch (std::exception& e)
	{
		cerr << e.what() << endl;
	}

	puts("Press any key to continue...");
	getc(stdin);
	return EXIT_SUCCESS;
}

string* buildPrompt()//Создание пользователя
{
	const int inputSize = 256;
	char inputBuf[inputSize] = { 0 };
	char nameBuf[inputSize] = { 0 };
	string* prompt = new string(": ");

	cout << "Please input a new username: ";
	cin.getline(nameBuf, inputSize);
	*prompt = (string)nameBuf + *prompt;
	boost::algorithm::to_lower(*prompt);

	return prompt;
}

void inboundLoop(socket_ptr sock, string_ptr prompt)//Отправка сообщения
{
	int bytesRead = 0;
	char readBuf[1024] = { 0 };

	for (;;)
	{
		if (sock->available())
		{
			bytesRead = sock->read_some(buffer(readBuf, inputSize));
			string_ptr msg(new string(readBuf, bytesRead));

			messageQueue->push(msg);
		}

		boost::this_thread::sleep(boost::posix_time::millisec(1000));
	}
}

void writeLoop(socket_ptr sock, string_ptr prompt)//Ввод сообщения
{
	char inputBuf[inputSize] = { 0 };
	string inputMsg;

	for (;;)
	{
		cin.getline(inputBuf, inputSize);
		inputMsg = *prompt + (string)inputBuf + '\n';

		if (!inputMsg.empty())
		{
			sock->write_some(buffer(inputMsg, inputSize));
		}

		if (inputMsg.find("exit") != string::npos)
			exit(1);

		inputMsg.clear();
		memset(inputBuf, 0, inputSize);
	}
}

void displayLoop(socket_ptr sock)//Отображение сообщения
{
	for (;;)
	{
		if (!messageQueue->empty())
		{
			if (!isOwnMessage(messageQueue->front()))
			{
				cout << "\n" + *(messageQueue->front());
			}

			messageQueue->pop();
		}

		boost::this_thread::sleep(boost::posix_time::millisec(1000));
	}
}

bool isOwnMessage(string_ptr message)//проверка на собственность сообщения
{
	if (message->find(*promptCpy) != string::npos)
		return true;
	else
		return false;
}