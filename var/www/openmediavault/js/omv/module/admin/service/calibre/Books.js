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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")

Ext.define("OMV.module.admin.service.calibre.Books", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],

    hidePagingToolbar : false,
    hideAddButton     : true,
    hideEditButton    : true,
    stateful          : true,
    stateId           : "a982a76d-6804-1332-b31b-8b48c0ea6dde",
    columns           : [{
        text      : _("ID"),
        sortable  : true,
        dataIndex : "id",
        stateId   : "id"
    },{
        text      : _("Title"),
        sortable  : true,
        dataIndex : "title",
        stateId   : "title"
    },{
        text      : _("Author"),
        sortable  : true,
        dataIndex : "author",
        stateId   : "author"
    }],

    initComponent : function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty  : "id",
                    fields      : [
                        { name : "id", type : "integer" },
                        { name : "title", type : "string" },
                        { name : "author", type : "string" }
                    ]
                }),
                proxy : {
                    type    : "rpc",
                    rpcData : {
                        service : "Calibre",
                        method  : "getBookList"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    doDeletion : function(record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Calibre",
                method  : "deleteBook",
                params  : {
                    id : record.get("id")
                }
            }
        });
    }

});

OMV.WorkspaceManager.registerPanel({
    id        : "books",
    path      : "/service/calibre",
    text      : _("Books"),
    position  : 20,
    className : "OMV.module.admin.service.calibre.Books"
});
