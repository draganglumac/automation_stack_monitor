#ifndef __SQL_CONNECTOR_H__
#define __SQL_CONNECTOR_H__
#include <jnxc_headers/jnxterm.h>
#include "sql_interface_layer.h"
void perform_store_sql_credentials(char* host_addr, char* user, char* pass);
int sql_query(char *query,mysql_result_bucket **result_bucket);
#endif
