#include <uWS/uWS.h>
#include <iostream>
#include "json.hpp"
#include "PID.h"
#include <math.h>

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != std::string::npos) {
    return "";
  }
  else if (b1 != std::string::npos && b2 != std::string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

int main()
{
  uWS::Hub h;

  PID pid;
  //double p[3] = {0.111222,0.0043165,1.05256};
  double p[3] = {0,0,0};
  double dp[3] = {1,1,1};
  pid.Init(p[0],p[1],p[2]);

  int n = 1;
  int max_n = 500;
  double total_cte = 0.0;
  double error = 0.0;
  double best_error = 10000.00;
  double tol = 0.01;
  int p_iterator = 0;
  int total_iterator = 0;
  int sub_move = 0;
  bool first = true;
  bool second = true;
  h.onMessage([&pid, &p, &dp, &n, &max_n, &tol, &error, &best_error, &p_iterator, &total_iterator, &total_cte, &first, &sub_move, &second](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
      auto s = hasData(std::string(data).substr(0, length));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
          // j[1] is the data JSON object
          double cte = std::stod(j[1]["cte"].get<std::string>());
          double speed = std::stod(j[1]["speed"].get<std::string>());
          double angle = std::stod(j[1]["steering_angle"].get<std::string>());
          double steer_value;
          double throttle_value = 0.3;
          
          
          if (n > max_n){ 
            if(total_iterator == 0) {
              best_error = total_cte/max_n;
            }
            std::cout << "iteration: " << total_iterator << " ";
            std::cout << "first: " << first << " ";
            std::cout << "second: " << second << " ";
            std::cout << "p_iterator: " << p_iterator << " ";
            std::cout << "sub_move: " << sub_move << " ";
            std::cout << "best_error: " << best_error << " ";
            std::cout << "p[0] p[1] p[2]: " << p[0] << " " << p[1] << " " << p[2] << " ";
            //double sump = p[0]+p[1]+p[2];
            //std::cout << "sump: " << sump << " ";
            if(first == true) {
              p[p_iterator] += dp[p_iterator];
              pid.Init(p[0], p[1], p[2]);
              first = false;
            }else{
              error = total_cte/max_n;
              std::cout << "error: " << error << " ";
              if(error < best_error) {
                  best_error = error;
                  dp[p_iterator] *= 1.1;
                  sub_move += 1;
                  first = true;
              }else{
                std::cout << "else: ";
                if(second == true) {
                  p[p_iterator] -= 2 * dp[p_iterator];
                  pid.Init(p[0], p[1], p[2]);
                  second = false;
                }else {
                  if(error < best_error) {
                      best_error = error;
                      dp[p_iterator] *= 1.1;
                      sub_move += 1;
                  }else {
                      p[p_iterator] += dp[p_iterator];
                      dp[p_iterator] *= 0.9;
                      sub_move += 1;
                  }
                  first = true;
                  second = true;
                }
              }
              
            }

            if(sub_move > 0) {
              p_iterator = p_iterator+1;
              sub_move = 0;
            }
            if(p_iterator == 3) {
              p_iterator = 0;
            }
            total_cte = 0.0;
            n = 0;
            total_iterator = total_iterator+1;
            
            std::string reset_msg = "42[\"reset\",{}]";
            ws.send(reset_msg.data(), reset_msg.length(), uWS::OpCode::TEXT);
            
          } else {
            
            //Steering value
            pid.UpdateError(cte);
            steer_value = pid.TotalError();
            // DEBUG
            //std::cout << "CTE: " << cte << " Steering Value: " << steer_value << " Throttle Value: " << throttle_value << " Count: " << n << std::endl;
            json msgJson;
            msgJson["steering_angle"] = steer_value;
            msgJson["throttle"] = throttle_value;
            auto msg = "42[\"steer\"," + msgJson.dump() + "]";
            //std::cout << msg << std::endl;
            ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

            total_cte = total_cte + pow(cte,2);
          }
          n = n+1;
        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }
  });

  // We don't need this since we're not using HTTP but if it's removed the program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1)
    {
      res->end(s.data(), s.length());
    }
    else
    {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port))
  {
    std::cout << "Listening to port " << port << std::endl;
  }
  else
  {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}