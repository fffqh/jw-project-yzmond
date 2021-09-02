#include <iostream>
#include <iomanip>
#include <mysql.h>
using namespace std;


int main()
{
    MYSQL     *mysql;   
    MYSQL_RES *result;   
    MYSQL_ROW  row;    

    /* ��ʼ�� mysql ������ʧ�ܷ���NULL */
    if ((mysql = mysql_init(NULL))==NULL) {
    	cout << "mysql_init failed" << endl;
    	return -1;
     }
    /* �������ݿ⣬ʧ�ܷ���NULL
       1��mysqldû����
       2��û��ָ�����Ƶ����ݿ���� */
    if (mysql_real_connect(mysql, "localhost", "dbuser_1851746", "yzmond.1851746","yzmon_1851746",0, NULL, 0)==NULL) {
    	cout << "mysql_real_connect failed(" << mysql_error(mysql) << ")" << endl;
    	return -1;
     }
    /* �����ַ���������������ַ����룬��ʹ/etc/my.cnf������Ҳ���� */
     mysql_set_character_set(mysql, "gbk");
    
    /* �������ݲ��룬�ɹ�����0�����ɹ���0 */

    const char* insert_base_sql = "insert into `devstate_base`"
    "(devstate_base_devid, devstate_base_devno, devstate_base_time, devstate_base_ipaddr, devstate_base_sid, devstate_base_type, devstate_base_version, devstate_base_cpu, devstate_base_sdram,  devstate_base_flash, devstate_base_ethnum, devstate_base_syncnum, devstate_base_asyncnum, devstate_base_switchnum, devstate_base_usbnum, devstate_base_prnnum, devstate_base_cpu_used, devstate_base_sdram_used, devstate_base_tty_configed, devstate_base_tty_connected, devstate_base_sendreq)"
    "values"
    "('1851746',            '1',               '2019-03-10 02:55:05', '192.168.89.1',     'fanqianhui-1746', 'fqh-6471581',      'Ver:17.46',           '200',             '200',                '200',               '2',                  '0',                   '16',                   '0',                     '1',                  '1',                  '0.2',                  '0.2',                    '10',                       default,                     default)";
    if(mysql_query(mysql, insert_base_sql)){
      cout << "mysql_query failed(" << mysql_error(mysql) << ")" << endl;
      return -1;
    }

    /* ���в�ѯ���ɹ�����0�����ɹ���0
       1����ѯ�ַ��������﷨����
       2����ѯ�����ڵ����ݱ� */
    if (mysql_query(mysql, "select * from devstate_base")) {
    	cout << "mysql_query failed(" << mysql_error(mysql) << ")" << endl;
    	return -1;
     }
    /* ����ѯ����洢���������ִ����򷵻�NULL
       ע�⣺��ѯ���ΪNULL�����᷵��NULL */
    if ((result = mysql_store_result(mysql))==NULL) {
    	cout << "mysql_store_result failed" << endl;
    	return -1;
     }
    /* ��ӡ��ǰ��ѯ���ļ�¼������ */
    cout << "select return " << (int)mysql_num_rows(result) << " records" << endl;

    /* �ͷ�result */
     mysql_free_result(result);
    /* �ر��������� */
     mysql_close(mysql);
    return 0;
}
