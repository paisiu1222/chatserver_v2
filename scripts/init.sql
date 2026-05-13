CREATE DATABASE IF NOT EXISTS chat;
USE chat;

CREATE TABLE IF NOT EXISTS user (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(256) NOT NULL,
    state ENUM('online', 'offline') DEFAULT 'offline'
);

CREATE TABLE IF NOT EXISTS friend (
    userid INT NOT NULL,
    friendid INT NOT NULL,
    PRIMARY KEY (userid, friendid),
    FOREIGN KEY (userid) REFERENCES user(id),
    FOREIGN KEY (friendid) REFERENCES user(id)
);

CREATE TABLE IF NOT EXISTS allgroup (
    id INT PRIMARY KEY AUTO_INCREMENT,
    groupname VARCHAR(50) NOT NULL,
    groupdesc VARCHAR(200) DEFAULT ''
);

CREATE TABLE IF NOT EXISTS groupuser (
    groupid INT NOT NULL,
    userid INT NOT NULL,
    grouprole ENUM('creator', 'normal') DEFAULT 'normal',
    PRIMARY KEY (groupid, userid),
    FOREIGN KEY (groupid) REFERENCES allgroup(id),
    FOREIGN KEY (userid) REFERENCES user(id)
);

CREATE TABLE IF NOT EXISTS offlinemessage (
    userid INT NOT NULL,
    message TEXT NOT NULL,
    FOREIGN KEY (userid) REFERENCES user(id)
);
