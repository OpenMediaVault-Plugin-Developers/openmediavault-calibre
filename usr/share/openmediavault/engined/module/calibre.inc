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

class OMVModuleCalibre extends \OMV\Engine\Module\ServiceAbstract
    implements \OMV\Engine\Notify\IListener, \OMV\Engine\Module\IServiceStatus

{
    public function getName()
    {
        return 'Calibre';
    }

    public function getStatus()
    {
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        $systemCtl = new \OMV\System\SystemCtl('calibre');
        return array(
            'name' => $this->getName(),
            'title' => gettext('Calibre'),
            'enabled' => $object->get('enable'),
            'running' => $systemCtl->isActive()
        );
    }

    public function applyConfig()
    {
        $cmd = new \OMV\System\Process('omv-mkconf', 'calibre');
        $cmd->setRedirect2to1();
        $cmd->execute();
    }

    public function startService()
    {
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.calibre');
        if (TRUE !== $object->get('enable'))
            return;
        // Start this service and enable the unit file.
        $systemCtl = new \OMV\System\SystemCtl('calibre');
        $systemCtl->enable(TRUE);
    }

    public function stopService()
    {
        $systemCtl = new \OMV\System\SystemCtl('calibre');
        $systemCtl->disable(TRUE);
    }

    final public function onSharedFolder($type, $path, $object)
    {
        $db = \OMV\Config\Database::getInstance();
        if (TRUE === $db->exists('conf.service.calibre', [
            'operator' => 'stringEquals',
            'arg0' => 'datasharedfolderref',
            'arg1' => $object['uuid']
        ])) {
            $this->setDirty();
        }
    }

    public function bindListeners(\OMV\Engine\Notify\Dispatcher $dispatcher)
    {
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            'org.openmediavault.conf.service.calibre',
            [ $this, 'setDirty' ]
        );
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            'org.openmediavault.conf.system.sharedfolder',
            [ $this, 'onSharedFolder' ]
        );
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            'org.openmediavault.conf.system.sharedfolder.privilege',
            [ $this, 'onSharedFolder' ]
        );
    }
}
