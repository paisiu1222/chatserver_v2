#include "usermodel.hpp"
#include "db.h"
#include "password_utils.hpp"
#include <iostream>
using namespace std;

// User表的增加方法
bool UserModel::insert(User &user)
{
    // 1.组装sql语句（密码 PBKDF2 哈希后存入，字符串参数转义防注入）
    std::string hashedPwd = hashPassword(user.getPwd());
    MySQL mysql;
    if (mysql.connect())
    {
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "insert into user(name, password, state) values('%s', '%s', '%s')",
                mysql.escape(user.getName()).c_str(),
                hashedPwd.c_str(),
                mysql.escape(user.getState()).c_str());

        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserModel::updateState(User user)
{
    // 1.组装sql语句
    MySQL mysql;
    if (mysql.connect())
    {
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "update user set state = '%s' where id = %d",
                mysql.escape(user.getState()).c_str(), user.getId());

        return mysql.update(sql);
    }
    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    // 1.组装sql语句
    const char *sql = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}