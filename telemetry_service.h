#pragma once

#include "telemetry.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <shared_mutex>
#include <mutex>

using telemetry::v1::GetParameterRequest;
using telemetry::v1::GetParameterResponse;
using telemetry::v1::SetParameterRequest;
using telemetry::v1::SetParameterResponse;
using telemetry::v1::TelemetryService;

class TelemetryServiceImpl final : public TelemetryService::Service {
public:
    TelemetryServiceImpl();
    ~TelemetryServiceImpl() override = default;

    grpc::Status GetParameter(grpc::ServerContext* context,
        const GetParameterRequest* request,
        GetParameterResponse* response) override;

    grpc::Status SetParameter(grpc::ServerContext* context,
        const SetParameterRequest* request,
        SetParameterResponse* response) override;

private:
    struct ParameterData {
        std::string value;
        google::protobuf::Timestamp timestamp;
    };

    
    std::unordered_map<std::string, ParameterData> parameters_;
    mutable std::shared_mutex params_mutex_; 

    
    std::unordered_set<std::string> processed_requests_;
    std::deque<std::string> request_history_;
    std::mutex requests_mutex_;
    const size_t MAX_HISTORY_SIZE = 10000;
    
    bool IsDuplicateRequest(const std::string& request_id);

    void LogInfo(const std::string& message) const;
    void LogError(const std::string& message) const;
};