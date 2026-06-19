#include "telemetry_service.h"
#include <google/protobuf/util/time_util.h>
#include <iostream>
#include <chrono>

TelemetryServiceImpl::TelemetryServiceImpl() {
    LogInfo("TelemetryService запущена.");
}

bool TelemetryServiceImpl::IsDuplicateRequest(const std::string& request_id) {
    if (request_id.empty()) return false;

    std::scoped_lock lock(requests_mutex_);
    if (processed_requests_.find(request_id) != processed_requests_.end()) {
        return true;
    }

    processed_requests_.insert(request_id);
    request_history_.push_back(request_id);

    if (request_history_.size() > MAX_HISTORY_SIZE) {
        processed_requests_.erase(request_history_.front());
        request_history_.pop_front();
    }
    return false;
}

void TelemetryServiceImpl::LogInfo(const std::string& message) const {
    std::cout << "[INFO] " << message << std::endl;
}

void TelemetryServiceImpl::LogError(const std::string& message) const {
    std::cerr << "[ERROR] " << message << std::endl;
}

grpc::Status TelemetryServiceImpl::GetParameter(grpc::ServerContext* context,
    const GetParameterRequest* request,
    GetParameterResponse* response) {
    if (request->name().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Parameter name is empty");
    }

    std::shared_lock<std::shared_mutex> lock(params_mutex_);

    auto it = parameters_.find(request->name());
    if (it == parameters_.end()) {
        response->set_status_code(static_cast<int32_t>(grpc::StatusCode::NOT_FOUND));
        response->set_status_message("Parameter not found"); 
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Parameter not found");
    }

    response->set_name(request->name());
    response->set_value(it->second.value);
    *response->mutable_timestamp() = it->second.timestamp;

    response->set_status_code(static_cast<int32_t>(grpc::StatusCode::OK));
    response->set_status_message("Success"); 

    return grpc::Status::OK;
}

grpc::Status TelemetryServiceImpl::SetParameter(grpc::ServerContext* context,
    const SetParameterRequest* request,
    SetParameterResponse* response) {
    if (request->name().empty() || request->request_id().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid arguments");
    }

    if (IsDuplicateRequest(request->request_id())) {
        response->set_status_code(static_cast<int32_t>(grpc::StatusCode::ALREADY_EXISTS));
        response->set_status_message("Request already processed"); 
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Request already processed");
    }

    ParameterData data;
    data.value = request->value();

    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;

    data.timestamp.set_seconds(seconds);
    data.timestamp.set_nanos(static_cast<int32_t>(nanos));

    {
        std::unique_lock<std::shared_mutex> lock(params_mutex_);
        parameters_[request->name()] = std::move(data);
    }

    response->set_status_code(static_cast<int32_t>(grpc::StatusCode::OK));
    response->set_status_message("Success"); 

    LogInfo("Установлен параметр: " + request->name());
    return grpc::Status::OK;
}