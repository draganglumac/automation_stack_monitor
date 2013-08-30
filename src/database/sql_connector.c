#include "sql_connector.h"
#include <stdio.h>
#include <stdlib.h>
char *host;
char *username;
char *password;

void perform_store_sql_credentials(char* host_addr, char* user, char* pass)
{
	host = host_addr;
	username = user;
	password = pass;
	printf("Set sql credentials %s %s %s \n",host,username,password);
}
int sql_query(char* query,mysql_result_bucket **result_bucket)
{
	MYSQL *connection = mysql_init(NULL);
	MYSQL_RES *result;
	if(connection == NULL) return 1;

	/* multi statements is useful for giving a string of commmands that are delimited with ; */
	if(mysql_real_connect(connection,host,username,password,0,0,NULL, CLIENT_MULTI_STATEMENTS) == NULL){ printf("CONNECTION ERROR\n"); return 1;};    
	int status = mysql_query(connection,query);
	if(status)
	{
		mysql_close(connection);
		printf("Problem with query\n");
		return 1;
	}
	/* process each statement result */
	do 
	{
		/* did current statement return data? */
		result = mysql_store_result(connection);
		if(result)
		{
			add_mysql_result_bucket_entry(result_bucket,result);
		//	mysql_free_result(result);
		}
		else          /* no result set or error */
		{
			if (mysql_field_count(connection) == 0)
			{
			}
			else  /* some error occurred */
			{
				printf("Could not retrieve result set\n");
				break;
			}
		}
		/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
		if ((status = mysql_next_result(connection)) > 0)
			printf("Could not execute statement in jnx_sql_query\n");
	} while (status == 0);
	mysql_close(connection);
	return 0;
}
