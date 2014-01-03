<?php

/**
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (C) 2009-2014 Volker Theile
 * @copyright Copyright (C) 2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once ("openmediavault/object.inc");
require_once ("openmediavault/error.inc");
require_once ("openmediavault/util.inc");
require_once ("openmediavault/rpc.inc");
require_once ("openmediavault/notify.inc");

class OMVRpcServiceCalibre extends OMVRpcServiceAbstract {
    private function getXpath() {
        return "//services/calibre";
    }

    public function getName() {
        return 'calibre';
    }

    public function initialize() {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
    }

    /**
     * Get main settings
     */
    function getSettings($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get configuration object
        $object = $xmlConfig->get( $this->getXpath() );
        if (is_null($object))
            throw new OMVException( OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $this->getXpath() );

        // Modify result data
        $object['enable']      = boolval($object['enable']);
        $object['port']        = intval($object['port']);
        $object['showtab']     = boolval($object['showtab']);

        return $object;
    }

    /**
     * Set main settings
     */
    function setSettings($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "enable"         : { "type" : "boolean" },
                "mntentref"      : { ' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . ' },
                "library-folder" : { "type" : "string", "optional" : true },
                "port"           : { "type" : "integer", "minimum" : 1 },
                "username"       : { "type" : "string", "optional" : true },
                "password"       : { "type" : "string", "optional" : true },
                "coversize"      : { "type" : "string", "optional" : true },
                "showtab"        : { "type" : "boolean" }
            }
        }');

        $xpath = sprintf("//system/fstab/mntent[uuid='%s']", $params['mntentref']);
        $deviceDir = $xmlConfig->get($xpath);
        $deviceDir = $deviceDir['dir'];

        $dataFolder = "{$deviceDir}/calibre-data";
        $params['library-folder'] = $dataFolder;

        // Prepare configuration data
        $object = array(
            "enable"         => array_boolval($params, 'enable'),
            "mntentref"      => $params['mntentref'],
            "library-folder" => $params['library-folder'],
            "port"           => intval($params['port']),
            "username"       => $params['username'],
            "password"       => $params['password'],
            "coversize"      => $params['coversize'],
            "showtab"        => array_boolval($params, 'showtab')
        );

        // Set configuration object
        if (false === $xmlConfig->replace($this->getXpath(), $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, $this->getXpath());

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, "org.openmediavault.services.calibre", $object);

        return $object;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceCalibre());
?>
