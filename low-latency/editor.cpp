/**
 * @file editor.cpp
 * MiNiFi - Low Latency Editor
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <fstream>
#include <map>

#include "server_http.hpp"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void respond_error(const std::shared_ptr<HttpServer::Response> &response, const std::shared_ptr<HttpServer::Request> &request, const std::string &reason) {
  *response << "HTTP/1.1 500 Internal Server Error\r\n"
            << "Content-Length: "
            << reason.length()
            << "\r\n\r\n"
            << reason;
}

void respond_notfound(const std::shared_ptr<HttpServer::Response> &response, const std::shared_ptr<HttpServer::Request> &request) {
  std::cerr << "not found: "
            << request->path
            << std::endl;
  *response << "HTTP/1.1 404 Not Found\r\n"
            << "Content-Length: 0\r\n\r\n";
}

void respond_static(const std::shared_ptr<HttpServer::Response> &response, const std::shared_ptr<HttpServer::Request> &request, const std::string &filename) {
  std::cerr << "serving static file: " 
            << filename
            << std::endl;
  
  struct stat ist;

  static std::map<std::string, std::string> mime_types{
    {"json", "application/json"},
    {"js", "text/javascript"},
    {"html", "text/html"},
    {"css", "text/css"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"gif", "image/gif"},
    {"ico", "image/x-icon"},
    {"ttf", "font/ttf"},
    {"woff2", "font/woff2"}
  };

  if (stat(filename.c_str(), &ist) == 0) {
    std::ifstream ifs(filename);

    if (ifs) {
      size_t ext_pos = filename.find_last_of(".");
      std::string file_ext;

      if (ext_pos != std::string::npos) {
        file_ext = filename.substr(filename.find_last_of(".") + 1);
      } else {
        std::cerr << "unknown file extension for static file: "
                  << filename
                  << std::endl;
        file_ext = "unknown";
      }

      *response << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: ";

      std::string default_mime_type = "application/octet-stream";

      if (mime_types.find(file_ext) != mime_types.end()) {
        *response << mime_types[file_ext];
      } else {
        std::cerr << "using default mime type "
                  << default_mime_type
                  << " for static file "
                  << filename
                  << std::endl;
        *response << default_mime_type;
      }

      *response << "\r\n"
                << "Content-Length: " << ist.st_size << "\r\n\r\n"
                << ifs.rdbuf();
    } else {
      return respond_error(response, request, "failed to read " + filename);
    }
  } else {
    return respond_notfound(response, request);
  }
}

int main() {

  std::cerr << "starting editor" << std::endl;

  HttpServer server;
  server.config.port = 20233;

  server.resource["^/$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::stringstream r;
    r << "Hello";
    response->write(r);
  };

  server.resource["^/nifi/$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    respond_static(response, request, "static/nifi/index.html");
  };

  const auto static_responder = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    // validate filename (NO ".." allowed)
    std::string static_filename = "static" + request->path;
    auto mpos = static_filename.find("..");

    if (mpos != std::string::npos) {
      return respond_notfound(response, request);
    } else {
      return respond_static(response, request, static_filename);
    }
    
    return respond_notfound(response, request);
  };

  server.resource["^/nifi/.+$"]["GET"] = static_responder;
  server.resource["^/nifi-docs/.+$"]["GET"] = static_responder;

  server.resource["^/nifi-api/flow/current-user$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"identity":"anonymous","anonymous":true,"provenancePermissions":{"canRead":true,"canWrite":true},"countersPermissions":{"canRead":true,"canWrite":true},"tenantsPermissions":{"canRead":true,"canWrite":true},"controllerPermissions":{"canRead":true,"canWrite":true},"policiesPermissions":{"canRead":true,"canWrite":true},"systemPermissions":{"canRead":true,"canWrite":true},"restrictedComponentsPermissions":{"canRead":true,"canWrite":true},"componentRestrictionPermissions":[{"requiredPermission":{"id":"access-keytab","label":"access keytab"},"permissions":{"canRead":true,"canWrite":true}},{"requiredPermission":{"id":"export-nifi-details","label":"export nifi details"},"permissions":{"canRead":true,"canWrite":true}},{"requiredPermission":{"id":"read-filesystem","label":"read filesystem"},"permissions":{"canRead":true,"canWrite":true}},{"requiredPermission":{"id":"execute-code","label":"execute code"},"permissions":{"canRead":true,"canWrite":true}},{"requiredPermission":{"id":"write-filesystem","label":"write filesystem"},"permissions":{"canRead":true,"canWrite":true}}],"canVersionFlows":false})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/client-id$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"(144aa51f-016b-1000-8550-6ae0a88ff4a3)";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: text/plain\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/config$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"flowConfiguration":{"supportsManagedAuthorizer":false,"supportsConfigurableAuthorizer":false,"supportsConfigurableUsersAndGroups":false,"autoRefreshIntervalSeconds":30,"currentTime":"14:26:33 EDT","timeOffset":-14400000,"defaultBackPressureObjectThreshold":10000,"defaultBackPressureDataSizeThreshold":"1 GB"}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/banners$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"banners":{"headerText":"","footerText":""}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/processor-types$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"processorTypes":[{"type":"org.apache.nifi.processors.stateful.analysis.AttributeRollingWindow","bundle":{"group":"org.apache.nifi","artifact":"nifi-stateful-analysis-nar","version":"1.10.0-SNAPSHOT"},"description":"Track a Rolling Window based on evaluating an Expression Language expression on each FlowFile and add that value to the processor's state. Each FlowFile will be emitted with the count of FlowFiles and total aggregate value of values processed in the current time window.","restricted":false,"tags":["rolling","data science","Attribute Expression Language","state","window"]}]})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/about$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"about":{"title":"MiNiFi - Low Latency","version":"0.0.1","uri":"http://localhost:20233/nifi-api/","contentViewerUrl":"../nifi-content-viewer/","timezone":"EDT","buildTag":"HEAD","buildRevision":"0","buildBranch":"master","buildTimestamp":"06/01/2019 09:53:19 EDT"}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/access/config$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"config":{"supportsLogin":false}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/controller-service-types$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"controllerServiceTypes":[]})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/reporting-task-types$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"reportingTaskTypes":[]})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/prioritizers$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"prioritizerTypes":[]})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/process-groups/root$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"permissions":{"canRead":true,"canWrite":true},"processGroupFlow":{"id":"1363d72a-016b-1000-63e9-ab675d872d0b","uri":"http://localhost:20233/nifi-api/flow/process-groups/1363d72a-016b-1000-63e9-ab675d872d0b","breadcrumb":{"id":"1363d72a-016b-1000-63e9-ab675d872d0b","permissions":{"canRead":true,"canWrite":true},"breadcrumb":{"id":"1363d72a-016b-1000-63e9-ab675d872d0b","name":"NiFi Flow"}},"flow":{"processGroups":[],"remoteProcessGroups":[],"processors":[],"inputPorts":[],"outputPorts":[],"connections":[],"labels":[],"funnels":[]},"lastRefreshed":"14:37:06 EDT"}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/process-groups/1363d72a-016b-1000-63e9-ab675d872d0b$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"permissions":{"canRead":true,"canWrite":true},"processGroupFlow":{"id":"1363d72a-016b-1000-63e9-ab675d872d0b","uri":"http://localhost:20233/nifi-api/flow/process-groups/1363d72a-016b-1000-63e9-ab675d872d0b","breadcrumb":{"id":"1363d72a-016b-1000-63e9-ab675d872d0b","permissions":{"canRead":true,"canWrite":true},"breadcrumb":{"id":"1363d72a-016b-1000-63e9-ab675d872d0b","name":"NiFi Flow"}},"flow":{"processGroups":[],"remoteProcessGroups":[],"processors":[],"inputPorts":[],"outputPorts":[],"connections":[],"labels":[],"funnels":[]},"lastRefreshed":"14:37:06 EDT"}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/status$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"controllerStatus":{"activeThreadCount":0,"terminatedThreadCount":0,"queued":"0 / 0 bytes","flowFilesQueued":0,"bytesQueued":0,"runningCount":0,"stoppedCount":2,"invalidCount":0,"disabledCount":0,"activeRemotePortCount":0,"inactiveRemotePortCount":0,"upToDateCount":0,"locallyModifiedCount":0,"staleCount":0,"locallyModifiedAndStaleCount":0,"syncFailureCount":0}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/controller/bulletins$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"bulletins":[],"controllerServiceBulletins":[],"reportingTaskBulletins":[]})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/flow/cluster/summary$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"({"clusterSummary":{"connectedNodeCount":0,"totalNodeCount":0,"connectedToCluster":false,"clustered":false}})";
    *response << "HTTP/1.1 200 OK\r\n"
              << "Content-Type: application/json\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/access/kerberos$"]["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"()";
    *response << "HTTP/1.1 409 Conflict\r\n"
              << "Content-Type: text/plain\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.resource["^/nifi-api/access/oidc/exchange$"]["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string doc = R"()";
    *response << "HTTP/1.1 409 Conflict\r\n"
              << "Content-Type: text/plain\r\n"
              << "Content-Length: " << doc.length() << "\r\n\r\n"
              << doc;
  };

  server.default_resource["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    respond_notfound(response, request);
  };

  server.default_resource["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    respond_notfound(response, request);
  };

  server.start();
  return 0;
}
