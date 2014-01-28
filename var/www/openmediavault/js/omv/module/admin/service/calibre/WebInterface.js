/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/panel/Panel.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")

Ext.define("OMV.module.admin.service.calibre.WebInterface", {
    extend : "OMV.workspace.panel.Panel",

    initComponent : function() {
        var me = this;

        OMV.Rpc.request({
            scope    : this,
            callback : function(id, success, response) {
                var link = "http://" + window.location.hostname + ":" + port;
                me.html = "<iframe src='" + link + "' width='100%' height='100%' />";
            },
            relayErrors : false,
            rpcData     : {
                service  : "Calibre",
                method   : "getSettings"
            }
        });

        me.callParent(arguments);
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "webclient",
    path      : "/service/calibre",
    text      : _("Web Interface"),
    position  : 30,
    className : "OMV.module.admin.service.calibre.WebInterface"
});
