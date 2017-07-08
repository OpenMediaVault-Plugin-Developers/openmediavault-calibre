#!/bin/sh

set -e

. /usr/share/openmediavault/scripts/helper-functions

if ! omv_config_exists "/config/services/calibre"; then
    CALIBRE_HOME=$(getent passwd calibre | cut -d: -f6)

    omv_config_add_node "/config/services" "calibre"
    omv_config_add_key "/config/services/calibre" "enable" "0"
    omv_config_add_key "/config/services/calibre" "datasharedfolderref" ""
    omv_config_add_key "/config/services/calibre" "port" "8080"
    omv_config_add_key "/config/services/calibre" "username" ""
    omv_config_add_key "/config/services/calibre" "password" ""
    omv_config_add_key "/config/services/calibre" "coversize" ""
    omv_config_add_key "/config/services/calibre" "importsharedfolderref" ""
fi

exit 0
