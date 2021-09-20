function edit_cancel() {
     $("#edit").html('');
 }

 function edit_submit() {
     fetch("/json", {
         method: "POST", 
         body: JSON.stringify({
             op: 'setcomp',
             ix: parseInt($(#n_index).val()),
             /*************
             last: $(#n_last).val(),
             first: $(#n_first).val(),
             birthyear: parseInt($(#n_yob).val()),
             belt: $(#n_grade).val(),
             club: $(#n_club).val(),
             country: $(#n_country).val(),
             regcat: $(#n_regcat).val(),
             category: $(#n_realcat).val(),
             weight: parseInt($(#n_weight).val()),
             seeding: parseInt($(#n_seeding).val()),
             clubseeding: parseInt($(#n_clubseeding).val()),
             id: $(#n_id).val(),
             coachid: $(#n_coachid).val(),
             gender: $(#n_gender).val(),
             control: $(#n_control).val(),
             hansokumake: $(#n_hansokumake).val(),
             comment: $(#n_comment).val(),
             ****/
         })
     })
         .then(response => response.json())
         .then(data => {
             //console.log('Success:', data);
         })
         .catch((error) => {
             alarm('Error:', error);
         });

     $("#edit").html('');
 }

 function edit_start(ix) {
     fetch("/json", {
         method: "POST", 
         body: JSON.stringify({
             op: 'sql',
             cmd: 'SELECT FROM competitors WHERE "index"='+ix+''
         })             
     })
         .then(response => response.json())
         .then(data => {
             console.log('Success:', data);
             
         })
         .catch((error) => {
             alarm('Error:', error);
         });
 }
