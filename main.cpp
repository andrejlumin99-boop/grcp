#include "telemetry_service.h"
#include "telemetry.grpc.pb.h" 
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <chrono>


void RunLocalTest() {
    
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n[ТЕСТ] Запуск локального тестирования" << std::endl;

    
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    auto stub = telemetry::v1::TelemetryService::NewStub(channel);

     
    {
        telemetry::v1::SetParameterRequest request;
        request.set_name("engine_rpm");
        request.set_value("3500");
        request.set_request_id("unique_id_12345");

        telemetry::v1::SetParameterResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub->SetParameter(&context, request, &response);

        if (status.ok() && response.status_code() == 0) {
            std::cout << "[ТЕСТ] Проверка SetParameter: УСПЕШНО (Добавлен параметр engine_rpm = 3500)" << std::endl;
        }
        else {
            std::cout << "[ТЕСТ] Проверка SetParameter: ОШИБКА! " << status.error_message() << std::endl;
        }
    }

    
    {
        telemetry::v1::SetParameterRequest request;
        request.set_name("engine_rpm");
        request.set_value("4000");
        request.set_request_id("unique_id_12345");

        telemetry::v1::SetParameterResponse response;
        grpc::ClientContext context;

        stub->SetParameter(&context, request, &response);

        if (response.status_code() == 6) {
            std::cout << "[ТЕСТ] Проверка дубликатов: УСПЕШНО (Сервер заблокировал повторный request_id)" << std::endl;
        }
        else {
            std::cout << "[ТЕСТ] Проверка дубликатов: ОШИБКА! Сервер пропустил дублирующийся ID запроса." << std::endl;
        }
    }

    
    {
        telemetry::v1::GetParameterRequest request;
        request.set_name("engine_rpm");

        telemetry::v1::GetParameterResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub->GetParameter(&context, request, &response);

        if (status.ok() && response.value() == "3500") {
            std::cout << "[ТЕСТ] Проверка GetParameter: УСПЕШНО (Успешно прочитано из памяти: "
                << response.name() << " = " << response.value() << ")" << std::endl;
        }
        else {
            std::cout << "[ТЕСТ] Проверка GetParameter: ОШИБКА! Не удалось получить верное значение." << std::endl;
        }
    }

    std::cout << "[ТЕСТ] Все локальные тесты успешно завершены.\n" << std::endl;
}

int main() {
    setlocale(LC_ALL, "rus");

    std::string address("0.0.0.0:50051");
    TelemetryServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Сервер запущен на адресе " << address << "..." << std::endl;

    
    std::thread test_thread(RunLocalTest);
    test_thread.detach();

    server->Wait(); 
    return 0;
}