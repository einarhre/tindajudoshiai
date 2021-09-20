"use strict";

var webSocket;

var po1 = ['Index', 'Last Name', 'First Name', 'Year of Birth', 'Grade', 'Club', 'Country',
           'Reg.Category', 'Weight', 'Id', 'Seeding', 'Club Seeding',
           // 12 ->
           'Tournament', 'Competitors', 'Categories', 'Drawing', 'Results', 'Preferences', 'Help',
           // 19 ->
           'New Competitor', 'Show Sheet', 'Draw Selected', 'Properties', 'Validate Database',
           // 24 ->
           'Print Selected Accreditation Cards', "Move Competitors to Category..."];
var po = po1;
var table;
var ajax_loading = false;

const MATCH_EXISTS      = 1;
const MATCH_MATCHED     = 2;
const MATCH_UNMATCHED   = 4;
const CAT_PRINTED       = 8;
const REAL_MATCH_EXISTS = 16;
const SYSTEM_DEFINED    = 32;

var cat2index = {};

Tabulator.prototype.extendModule("moveRow", "receivers", {
    nameUpdate:function(fromRow, toRow, fromTable){
        console.log('nameUpdate:', fromRow, toRow);
        if(toRow){
            toRow.update({"name":fromRow.getData().name});
            return true;
        }
        return false;
    }
});

function deselect_all() {
    let selectedRows = table.getSelectedRows();
    selectedRows.forEach(r => {
        table.deselectRow(r);
    });
}

function get_cat_color(cat) {
    let s = cat2index[cat].status;
    if (((s & SYSTEM_DEFINED) && (s & MATCH_UNMATCHED) == 0)) {
        return "green";
    } else if (s & MATCH_MATCHED) {
        return "orange";
    } else if (s & REAL_MATCH_EXISTS) {
        return "yellow";
    }
    return "white";
}

function competitor_edit(c) {
    $("#popup").load("editcomp.html", function(response, status, xhr) {
        if (status == 'success')
            edit_start(c.index);
    });
}

function show_sheet(c) {
    try {
        let ix = c.index;
        if (ix < 10000) {
            ix = cat2index[c.category].index;
        }
        window.open("/web?op=5&s=2&c="+ix);
    } catch(err) {
    }
}

function accreditation_cards() {
    let comps = [];
    var selectedData = table.getSelectedData();
    selectedData.forEach(c => {
        if (c.index < 10000)
            comps.push(parseInt(c.index));
    });
    open_pdf(JSON.stringify({ op: 'accrcard', comps: comps, what: 0 }));
    deselect_all();
}

function move_done(data, affected) {
    affected.forEach(c => {
        get_competitors(c);
    });
    
    if (data.length == 0)
        return;
    let msg = '';
    data.forEach(m => {
        msg += m.first+' '+m.last+': '+m.msg+'\n';
    });
    alert(msg);
}

function move_to_cat(cat) {
    console.log("MOVE to ", cat);
    var affected = new Set();
    affected.add(cat);
    let comps = [];
    var selectedData = table.getSelectedData();
    selectedData.forEach(c => {
        if (c.index < 10000) {
            comps.push(parseInt(c.index));
            affected.add(c.category);
        }
    });
    
    json_send(JSON.stringify({ op: 'movcat', comps: comps, dest: cat }), move_done, affected);
    deselect_all();
}

function draw_done(arg) {
    get_categories();
}

function draw_selected() {
    let cats = [];
    var selectedData = table.getSelectedData();
    selectedData.forEach(c => {
        if (c.index >= 10000)
            cats.push(parseInt(c.index));
    });
    //console.log('SELECTED:', cats);
    json_send(JSON.stringify({ op: 'draw', cat: cats }), draw_done, '');
    deselect_all();
}

function undraw_selected() {
    json_send(JSON.stringify({ op: 'undraw' }), draw_done, '');
}

var headerMenu = [
    {
        label:"",
        action:function(e, column){
            let selectedRows = table.getSelectedRows();
            selectedRows.forEach(r => {
                console.log('row', r.getData().index);
                table.deselectRow(r);
            });
        }
    },
];

var competitors_row_menu = [
    {
        label:po[25],
        menu: [
            {
                label:"-77",
                action:function(e, row){
                }
            },
        ]
    },
    {
        label:"Edit",
        action:function(e, row){
            //console.log('EDIT:', row.getData());
            competitor_edit(row.getData());
        }
    },
    {
        label:po[20],
        action:function(e, row){
            show_sheet(row.getData());
        }
    },
    {
        label:po[21],
        action:function(e, row){
            draw_selected();
        }
    },
    {
        label:po[24],
        action:function(e, row){
            accreditation_cards();
        }
    },
];

function create_table() {
    var competitors_columns = [
        {title:po[0], field:"index", sorter:"number", visible:false},
        {title:po[1], field:"last", sorter:"string", editor:false, headerMenu:headerMenu},
        {title:po[2], field:"first", sorter:"string", editor:false},
        {title:po[3], field:"yob", sorter:"number", editor:false},
        {title:po[4], field:"grade", sorter:"string", editor:true},
        {title:po[5], field:"club", sorter:"string", editor:true},
        {title:po[6], field:"country", sorter:"string", editor:true},
        {title:'Cat', field:"category", sorter:"string"},
        {title:po[7], field:"regcat", sorter:"string", editor:true},
        {title:po[8], field:"weight", sorter:"number", editor:true},
        {title:po[9], field:"id", sorter:"string", editor:true},
        {title:po[10], field:"seeding", sorter:"number", editor:false},
        {title:po[11], field:"clubseeding", sorter:"number", editor:false},
        {title:'Status', field:"status", sorter:"number", visible:false},
    ];

    //define table
    table = new Tabulator("#competitors", {
        index: "index",
        dataTree:true,
        selectable:true, //make rows selectable
        columns: competitors_columns,
        //movableRows: true, //enable user movable rows
        //groupBy: "category",
        rowFormatter:function(row) {
            var s = row.getData().status;
            //console.log('status:', s);
            if (((s & SYSTEM_DEFINED) && (s & MATCH_UNMATCHED) == 0)) {
                row.getElement().style.backgroundColor = "green";
            } else if (s & MATCH_MATCHED) {
                row.getElement().style.backgroundColor = "orange";
            } else if (s & REAL_MATCH_EXISTS) {
                row.getElement().style.backgroundColor = "yellow";
            }
        },
        tableBuilt:function(){
            get_categories();
        },
        /*rowClick: function(e, row) {
            alert("Row " + row.getData().index + " Clicked!!!!");
        },*/
        rowSelectionChanged:function(data, rows){
            //console.log('SELECT:', data, rows);
            //update selected row counter on selection change
    	    //document.getElementById("select-stats").innerHTML = data.length;
        },
        rowContextMenu: competitors_row_menu,
    });
}

function set_category_list(data) {
     cat2index = {};
     data.forEach(function(c) {
         cat2index[c.category] = c;
     });
}

function get_categories() {
    send_json(JSON.stringify({ op: 'categories' }), 1);
}

function get_competitors(cat) {
    send_json(JSON.stringify(
        { op: 'sql',
          cmd: 'SELECT * FROM competitors WHERE category="'+cat+'"'
        }), 2, cat);
}

function get_words() {
    send_json(JSON.stringify(
        { op: 'lang',
          words: po1
        }), 3);
}

function update_competitors(catix, data) {
    var comps = [];

    for (let i = 1; i < data.length; i++) {
        var c = data[i];
        var comp = {
            index: c[0],
            last: c[1],
            first: c[2],
            yob: c[3],
            grade: c[4],
            club: c[5],
            regcat: c[6],
            weight: c[7],
            //visible: c[8],
            category: c[9],
            //deleted: c[10],
            country: c[11],
            id: c[12],
            seeding: c[13],
            clubseeding: c[14],
            //comment: c[15],
            //coachid: c[16],
        }
        //console.log('COMP:', comp);
        comps.push(comp);
    };        
    console.log('ID:', catix, comps);
    table.updateOrAddData([{index: catix, _children: comps}]);
}

function rec_json(data, what, arg) {
    if (what == 1) {
        // categories
        var menu = [{label:'?', action: function(e, row){ move_to_cat('?');}}]
        var cats = [];
        cat2index = {};
        cat2index['?'] = {index:999999, category:'?', status:0 };
        table.clearData();
        table.addData([{index:999999, last:'?', first:'', yob:'', grade:'', status:0,  _children:[] }]);
        
        data.forEach(function(c) {
            //console.log('cat:', c);
            cat2index[c.category] = c;
            let cat = {
                index: c.index,
                last: c.category,
                first: 'system',
                yob: '[' + c.numcomp + ']',
                grade: 'T'+c.tatami,
                status: c.status,
            };
            table.addData([cat]);
        });

        //table.setData(cats);
        
        data.forEach(function(c) {
            get_competitors(c['category'], 2);
            menu.push({label:c.category, action: function(e, row){move_to_cat(c.category);} });
        });

        competitors_row_menu[0].menu = menu;
    } else if (what == 2) {
        if (arg == '?')
            update_competitors(999999, data);
        else
            update_competitors(cat2index[arg].index, data);
    } else if (what == 3) {
        po = data;
        for (let i = 0; i < 7; i++) {
            $('#m'+(i+1)).html(po[i+12]);
        }
        $('#m1-1').html(po[22]);
        $('#m1-5').html(po[23]);
        $('#m2-1').html(po[19]);

        competitors_row_menu[1].label = po[20];
        competitors_row_menu[2].label = po[21];
        competitors_row_menu[3].label = po[24];
        
        create_table();
    }
}

function send_json(reqjson, what, arg='') {
    //console.log('SEND:', reqjson);
    fetch("/json", {
        method: "POST", 
        body: reqjson
    })
        .then(response => response.json())
        .then(data => {
            //console.log('Success:', data);
            rec_json(data, what, arg);
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function json_send(reqjson, cb, arg='') {
    console.log('SEND:', reqjson);
    fetch("/json", {
        method: "POST", 
        body: reqjson
    })
        .then(response => response.json())
        .then(data => {
            //console.log('Success:', data);
            if (cb) cb(data, arg);
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

function open_pdf(reqjson) {
    fetch("/json", {
        method: "POST", 
        body: reqjson
    })
        .then( res => res.blob() )
        .then( blob => {
            var file = window.URL.createObjectURL(blob);
            //window.location.assign(file);
            window.open(file);
        });
}

function Aopen_pdf(reqjson) {
    console.log('OPEN_PDF:', reqjson);
    try {
        fetch("/json", {
            method: "POST", 
            body: reqjson
        })
            .then(
                data => {
                    return data.blob();
                }
            ).then(
                response => {
                    console.log(response.type);
                    const dataType = response.type;
                    const binaryData = [];
                    binaryData.push(response);
                    const downloadLink = document.createElement('a');
                    downloadLink.href = window.URL.createObjectURL(new Blob(binaryData, { type: dataType }));
                    downloadLink.setAttribute('download', 'report');
                    document.body.appendChild(downloadLink);
                    downloadLink.click();
                    downloadLink.remove();
                }
            )

    } catch (e) {
        addToast('Error inesperado.', {appearance: 'error', autoDismiss: true});
    }
}

get_words();
get_competitors('?');
