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

    public function getName() {
        return 'calibre';
    }

    private function getPluginName() {
        return strtolower( $this->getName() );
    }

    private function getXPath() {
        $path = sprintf( "//services/%s", $this->getPluginName() );
        return $path;
    }

    private function getEventMessagePath() {
        $path = sprintf( "org.openmediavault.services.%s", $this->getPluginName() );
        return $path;
    }

    private function getProgram() {
        return "/opt/calibre/calibredb";
    }

    public function initialize() {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
        $this->registerMethod('doImport');
        $this->registerMethod('getBookList');
        $this->registerMethod('deleteBook');
        $this->registerMethod('doUpdate');
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
        $object['enable']  = boolval($object['enable']);
        $object['port']    = intval($object['port']);
        $object['showtab'] = boolval($object['showtab']);

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
                "enable"          : { "type" : "boolean" },
                "mntentref"       : { ' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . ' },
                "library-folder"  : { "type" : "string", "optional" : true },
                "port"            : { "type" : "integer", "minimum" : 1 },
                "username"        : { "type" : "string", "optional" : true },
                "password"        : { "type" : "string", "optional" : true },
                "coversize"       : { "type" : "string", "optional" : true },
                "showtab"         : { "type" : "boolean" },
                "sharedfolderref" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_NONE'].', "optional" : true }
            }
        }');

        $xpath = sprintf("//system/fstab/mntent[uuid='%s']", $params['mntentref']);
        $deviceDir = $xmlConfig->get($xpath);
        $deviceDir = $deviceDir['dir'];

        $dataFolder = "{$deviceDir}/calibre-data";
        $params['library-folder'] = $dataFolder;

        // Prepare configuration data
        $object = array(
            "enable"          => array_boolval($params, 'enable'),
            "mntentref"       => $params['mntentref'],
            "library-folder"  => $params['library-folder'],
            "port"            => intval($params['port']),
            "username"        => $params['username'],
            "password"        => $params['password'],
            "coversize"       => $params['coversize'],
            "showtab"         => array_boolval($params, 'showtab'),
            "sharedfolderref" => $params['sharedfolderref']
        );

        // Set configuration object
        if (false === $xmlConfig->replace($this->getXpath(), $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, $this->getXpath());

        // Notify configuration changes
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, $this->getEventMessagePath(), $object);

        return $object;
    }

    public function doImport($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "sharedfolderref" : { "type" : "string" }
            }
        }');

        $sfref = $params['sharedfolderref'];

        if( strlen($sfref) < 30 )
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, "Please set shared folder to import books.");

        $object = $xmlConfig->get( $this->getXpath() );
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        $sfPath = OMVRpc::exec("ShareMgmt", "getPath", array("uuid" => $sfref), $context);

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = $this->getProgram() . " add --recurse --library-path \"" . $object['library-folder'] . "\" $sfPath 2>&1";

            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function getBookList($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        $object = $xmlConfig->get( $this->getXpath() );
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        $cmd = $this->getProgram() . " list --library-path \"" . $object['library-folder'] . "\" -f title,authors --separator \"\t\" 2>&1";
        OMVUtil::exec($cmd, $output, $result);

        if ($result !== 0)
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));

        // Get column names
        $cols = array("id","title","author");
        $split_limit = count($cols);
        array_shift($output);

        $objects = array();
        foreach ($output as $line) {
            if (intval(trim($line[0])) < 1)
                continue;

            $objects[] = array_combine($cols, preg_split('/\t/', trim($line), $split_limit));
        }

        return array(
            "total" => count($objects),
            "data"  => $objects
        );
    }

    public function deleteBook($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "id" : { "type" : "integer", "minimum" : 1 }
            }
        }');

        $object = $xmlConfig->get( $this->getXpath() );
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        $cmd = $this->getProgram() . " remove --library-path \"" . $object['library-folder'] . "\" " . $params['id'] . " 2>&1";
        if(0 !== $this->exec($cmd, $output))
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
    }

    function doUpdate($params, $context) {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = "omv-mkconf calibre update";
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceCalibre());
