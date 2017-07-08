<?php
/**
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (C) 2009-2014 Volker Theile
 * @copyright Copyright (C) 2014-2017 OpenMediaVault Plugin Developers
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

class OMVRpcServiceCalibre extends \OMV\Rpc\ServiceAbstract
{
    private $calibredb = '/opt/calibre/calibredb';

    public function getName()
    {
        return 'calibre';
    }

    public function initialize()
    {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');

        $this->registerMethod('getBookList');
        $this->registerMethod('deleteBook');

        $this->registerMethod('doImport');
        $this->registerMethod('doUpdate');
    }

    function getSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        return $object->getAssoc();
    }

    function setSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.calibre.setsettings');
        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        $object->setAssoc($params);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function getBookList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        $dataRef = $object->get('datasharedfolderref');
        $objects = [];

        if (is_uuid($dataRef)) {
            $dataPath = Rpc::call('ShareMgmt', 'getPath', ['uuid' => $dataRef], $context);

            $cmd = sprintf('%s list --library-path \"%s\" -f title,authors --separator \"\t\"',
                $this->calibredb, $dataPath
            );
            $process = new \OMV\System\Process($cmd);
            $process->execute($output, $result);

            // Get column names
            $cols = ['id','title','author'];
            $split_limit = count($cols);
            array_shift($output);

            foreach ($output as $line) {
                if (intval(trim($line[0])) < 1)
                    continue;

                $objects[] = array_combine($cols, preg_split('/\t/', trim($line), $split_limit));
            }
        }
        return [
            'total' => count($objects),
            'data'  => $objects
        ];
    }

    public function deleteBook($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.calibre.deletebook');

        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        $dataRef = $object->get('datasharedfolderref');
        $dataPath = Rpc::call('ShareMgmt', 'getPath', ['uuid' => $dataRef,], $context);

        $cmd = sprintf('%s remove --library-path \"%s\" %s',
            $this->calibredb, $dataPath, $params['id']
        );
        $process = new \OMV\System\Process($cmd);
        $process->execute($output, $result);
    }

    public function doImport($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.calibre.doimport');

        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        $dataRef = $object->get('datasharedfolderref');
        $importRef = $params['sharedfolderref'];

        if ( $importRef == $dataRef ) {
            throw new \OMV\Exception(
                gettext("Import directory cannot be the same as the data directory.")
            );
        }
        $importPath = Rpc::call('ShareMgmt', 'getPath', ['uuid' => $importRef,], $context);
        $dataPath = Rpc::call('ShareMgmt', 'getPath', ['uuid' => $dataRef,], $context);

        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($dataPath, $importPath) {
                $cmdArgs = [];
                $cmdArgs[] = 'add';
                $cmdArgs[] = '--recurse';
                $cmdArgs[] = sprintf('--library-path \"%s\"', $dataPath);
                $cmdArgs[] = sprintf('\"%s\"', $importPath);
                $cmd = new \OMV\System\Process($this->calibredb, $cmdArgs);
                $cmdLine = $cmd->getCommandLine();
                $this->exec($cmdLine, $output, $bgOutputFilename);
                return $output;
            }
        );
    }

    function doUpdate($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
            use ($params) {
                $cmd = new \OMV\System\Process('omv-mkconf', 'calibre', 'update');
                $cmdLine = $cmd->getCommandLine();
                $this->exec($cmdLine, $output, $bgOutputFilename);
                return $output;
            }
        );
    }
}
