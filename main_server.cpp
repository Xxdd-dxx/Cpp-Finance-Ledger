#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <exception>

#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#include "include/httplib.h"
#include "include/json.hpp"

using json = nlohmann::json;
using namespace soci;
using namespace std;

// 数据库连接配置
const string DB_CONN_STR = "db=finance_db user=root password=your_password host=127.0.0.1 charset=utf8mb4";

struct User { int id; string username; string password; };
struct Account { int id; int user_id; string account_name; double balance; string account_type; };
struct Transaction { 
    int id; int user_id; int account_id; 
    double amount; string tx_type; string category; 
    string tx_date; string description; 
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Account, id, user_id, account_name, balance, account_type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transaction, id, user_id, account_id, amount, tx_type, category, tx_date, description)

namespace soci {
    template<> struct type_conversion<Account> {
        typedef values base_type;
        static void from_base(values const & v, indicator, Account & p) {
            p.id = v.get<int>("id"); p.user_id = v.get<int>("user_id");
            p.account_name = v.get<string>("account_name"); p.balance = v.get<double>("balance");
            p.account_type = v.get<string>("account_type");
        }
        static void to_base(const Account & p, values & v, indicator & ind) {
            v.set("user_id", p.user_id); v.set("account_name", p.account_name);
            v.set("balance", p.balance); v.set("account_type", p.account_type); ind = i_ok;
        }
    };

    template<> struct type_conversion<Transaction> {
        typedef values base_type;
        static void from_base(values const & v, indicator, Transaction & p) {
            p.id = v.get<int>("id"); p.user_id = v.get<int>("user_id"); p.account_id = v.get<int>("account_id");
            p.amount = v.get<double>("amount"); p.tx_type = v.get<string>("tx_type");
            p.category = v.get<string>("category"); 
            p.description = v.get_indicator("description") == i_null ? "" : v.get<string>("description");
            std::tm t = v.get<std::tm>("tx_date"); char buffer[11]; strftime(buffer, sizeof(buffer), "%Y-%m-%d", &t); p.tx_date = string(buffer);
        }
        static void to_base(const Transaction & p, values & v, indicator & ind) {
            v.set("user_id", p.user_id); v.set("account_id", p.account_id);
            v.set("amount", p.amount); v.set("tx_type", p.tx_type);
            v.set("category", p.category); v.set("tx_date", p.tx_date); v.set("description", p.description); ind = i_ok;
        }
    };
}

void send_json_response(httplib::Response& res, int status, const json& body) {
    res.status = status; res.set_header("Access-Control-Allow-Origin", "*"); res.set_content(body.dump(), "application/json");
}

int main() {
    httplib::Server svr;

    svr.Get("/api/avatar/get", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("username")) return send_json_response(res, 400, {{"error", "missing username"}});
        try {
            string user = req.get_param_value("username"); session sql(mysql, DB_CONN_STR);
            string avatar = ""; indicator ind;
            sql << "SELECT avatar FROM users WHERE username = :u", use(user), into(avatar, ind);
            if (ind == i_ok && !avatar.empty()) { send_json_response(res, 200, {{"avatar", avatar}}); } 
            else { send_json_response(res, 404, {{"error", "no avatar"}}); }
        } catch (...) { send_json_response(res, 500, {{"error", "db error"}}); }
    });

    svr.Post("/api/avatar/update", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            int user_id = j.at("user_id").get<int>(); string avatar = j.at("avatar").get<string>(); 
            session sql(mysql, DB_CONN_STR);
            sql << "UPDATE users SET avatar = :a WHERE id = :id", use(avatar), use(user_id);
            send_json_response(res, 200, {{"message", "更新成功"}});
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    svr.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            string user = j.at("username").get<string>(); string pass = j.at("password").get<string>();
            string avatar = j.value("avatar", ""); 
            if (user.empty() || pass.empty()) return send_json_response(res, 400, {{"error", "不能为空"}});

            session sql(mysql, DB_CONN_STR); int count = 0;
            sql << "SELECT COUNT(*) FROM users WHERE username = :u", use(user), into(count);
            if (count > 0) return send_json_response(res, 400, {{"error", "已被注册"}});

            sql << "INSERT INTO users (username, password, avatar) VALUES (:u, :p, :a)", use(user), use(pass), use(avatar);
            send_json_response(res, 201, {{"message", "注册成功"}});
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            string user = j.at("username").get<string>(); string pass = j.at("password").get<string>();
            session sql(mysql, DB_CONN_STR); int count = 0;
            sql << "SELECT COUNT(*) FROM users WHERE username = :u AND password = :p", use(user), use(pass), into(count);
            if (count > 0) {
                int user_id = 0; sql << "SELECT id FROM users WHERE username = :u", use(user), into(user_id);
                send_json_response(res, 200, {{"message", "登录成功"}, {"user_id", user_id}});
            } else { send_json_response(res, 401, {{"error", "用户名或密码错误"}}); }
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    svr.Post("/api/accounts/create", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            int user_id = j.at("user_id").get<int>(); string account_name = j.at("account_name").get<string>();
            double balance = j.at("balance").get<double>(); string account_type = j.at("account_type").get<string>();

            session sql(mysql, DB_CONN_STR);
            sql << "INSERT INTO accounts (user_id, account_name, balance, account_type) VALUES (:user_id, :account_name, :balance, :account_type)", 
                   use(user_id), use(account_name), use(balance), use(account_type);
            send_json_response(res, 201, {{"message", "成功"}});
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    svr.Get("/api/accounts", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("user_id")) return send_json_response(res, 400, {{"error", "缺少参数"}});
        try {
            int uid = stoi(req.get_param_value("user_id")); session sql(mysql, DB_CONN_STR);
            rowset<Account> rs = (sql.prepare << "SELECT id, user_id, account_name, balance, account_type FROM accounts WHERE user_id = :uid", use(uid));
            vector<Account> accounts; for (auto it = rs.begin(); it != rs.end(); ++it) accounts.push_back(*it);
            send_json_response(res, 200, json(accounts));
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    // ========================================================
    // 【新增】拉取指定账户的详细流水，用于双击查看
    // ========================================================
    svr.Get("/api/transactions", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("user_id") || !req.has_param("account_id")) return send_json_response(res, 400, {{"error", "缺少参数"}});
        try {
            int uid = stoi(req.get_param_value("user_id"));
            int aid = stoi(req.get_param_value("account_id"));
            session sql(mysql, DB_CONN_STR);
            // 拉取特定账户的所有流水
            rowset<Transaction> rs = (sql.prepare << 
                "SELECT id, user_id, account_id, amount, tx_type, category, tx_date, description "
                "FROM transactions WHERE user_id = :uid AND account_id = :aid ORDER BY tx_date DESC", use(uid), use(aid));
            vector<Transaction> txs;
            for (auto it = rs.begin(); it != rs.end(); ++it) txs.push_back(*it);
            send_json_response(res, 200, json(txs));
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    svr.Post("/api/import/transactions", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j_array = json::parse(req.body); session sql(mysql, DB_CONN_STR); sql.begin(); 
            for (auto& item : j_array) {
                int user_id = item.at("user_id").get<int>(); int account_id = item.at("account_id").get<int>();
                double amount = item.value("amount", 0.0); string tx_type = item.at("tx_type").get<string>();
                string category = item.at("category").get<string>(); string tx_date = item.at("tx_date").get<string>();
                string description = item.value("description", "");

                sql << "INSERT INTO transactions (user_id, account_id, amount, tx_type, category, tx_date, description) VALUES (:user_id, :account_id, :amount, :tx_type, :category, :tx_date, :description)", 
                       use(user_id), use(account_id), use(amount), use(tx_type), use(category), use(tx_date), use(description);
                
                if (tx_type == "INCOME") { sql << "UPDATE accounts SET balance = balance + :amount WHERE id = :id", use(amount), use(account_id); } 
                else { sql << "UPDATE accounts SET balance = balance - :amount WHERE id = :id", use(amount), use(account_id); }
            }
            sql.commit(); send_json_response(res, 201, {{"message", "操作成功"}});
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    svr.Get("/api/export/csv", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("user_id")) return send_json_response(res, 400, {{"error", "缺少参数"}});
        try {
            int uid = stoi(req.get_param_value("user_id")); session sql(mysql, DB_CONN_STR);
            rowset<Transaction> rs = (sql.prepare << "SELECT id, user_id, account_id, amount, tx_type, category, tx_date, description FROM transactions WHERE user_id = :uid ORDER BY tx_date DESC", use(uid));
            stringstream csv; csv << "交易ID,账户ID,日期,分类,类型,金额,备注\n";
            for (auto it = rs.begin(); it != rs.end(); ++it) {
                csv << it->id << "," << it->account_id << "," << it->tx_date << "," << it->category << "," << (it->tx_type == "INCOME" ? "收入" : "支出") << "," << it->amount << "," << it->description << "\n";
            }
            res.status = 200; res.set_header("Content-Disposition", "attachment; filename=\"finance_export.csv\""); res.set_content(csv.str(), "text/csv; charset=utf-8");
        } catch (const exception& e) { send_json_response(res, 500, {{"error", e.what()}}); }
    });

    cout << "===== 启动完成：包含流水查询功能 =====" << endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}