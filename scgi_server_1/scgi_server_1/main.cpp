#include <string.h>
#include <cstdlib>
#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <thread>

using boost::asio::ip::tcp;


class server_action{
public:
    server_action(boost::asio::io_context& io_context): socket_(io_context){}
    
    tcp::socket& socket(){
        return socket_;
    }

    void start(){
        //читаем данные от клиента
        std::cout << "читаем данные от клиента\n";
        //sleep(2);
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&server_action::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        //printf("%s", data_);
    }

private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred){
        if (!error){
            printf("%s\n", data_);
            
            //пишем ответ клиенту
            std::cout << "пишем ответ клиенту\n";
            //strcpy(data_, "\"Status: 200 OK\" <0d 0a>\n \"Content-Type: text/plain\" <0d 0a>\n \"\" <0d 0a> \"42\"\n");
            strcpy(data_, "HTTP/1.1 200 Vse rabotaet");
            //printf("%s", data_);
            //sleep(2);
            boost::asio::async_write(socket_, boost::asio::buffer(data_, bytes_transferred),
                                     boost::bind(&server_action::handle_write, this,
                                                 boost::asio::placeholders::error));
            //sleep(2);
        } else {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error){
        if (!error){
            //закрываем соединение после ответа
            std::cout << "закрываем соединение после ответа\n";
            socket_.close();
            delete this;
        } else {
            delete this;
        }
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};




class server{
public:
    server(boost::asio::io_context& io_context, short port): io_context_(io_context), acceptor_(io_context, tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port)){
        start_accept();
    }

private:
    void start_accept(){
        //принимаем новое соединение
        std::cout << "принимаем новое соединение\n";
        server_action* new_action = new server_action(io_context_);
        acceptor_.async_accept(new_action->socket(),
                               boost::bind(&server::handle_accept, this, new_action,
                                           boost::asio::placeholders::error));
  }

    void handle_accept(server_action* new_session, const boost::system::error_code& error){
        if (!error){
            new_session->start();
        } else {
            delete new_session;
        }

        start_accept();
  }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};



boost::asio::io_context context;

void run_context(){
    context.run();
}


void handle_signal(const boost::system::error_code & err, int signal){
    if (!err){
        std::cerr << "\nExit signal\n";
        exit(0);
    }
}





int main(int argc, char* argv[]){
    try{
        if (argc != 3)
        {
            std::cerr << "Не все параметры\n";
            return 1;
        }
        
        int num_threads = std::atoi(argv[2]);
        
        if (num_threads > std::thread::hardware_concurrency())
        {
            std::cerr << "Система не поддерживает столько потоков\n";
            return 1;
        }
        
        //std::cout << std::thread::hardware_concurrency() << "\n";
        
        boost::asio::signal_set sig(context, SIGINT);
        sig.async_wait(handle_signal);
        
        
        server s(context, std::atoi(argv[1]));
        
        std::vector<std::thread> threads;
        for ( int i = 0; i < num_threads; i++)
        {
            threads.push_back(std::thread(run_context));
        }
        
        for (std::thread &t: threads)
        {
              if (t.joinable())
              {
                t.join();
              }
        }
        
    }
    catch (std::exception& e){
        std::cerr << "Исключение: " << e.what() << "\n";
    }

    return 0;
}
