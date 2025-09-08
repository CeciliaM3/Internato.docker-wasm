document.addEventListener("DOMContentLoaded", function () {
  document.querySelectorAll('.select-option').forEach(option => {
    option.addEventListener('click', function() {
      if (option.classList.contains("currently_selected")) {
        document.getElementById('realurl_container').dataset.realurl = "undefined";
        option.classList.remove("currently_selected");
      } 
      else {
        let urltemplate = option.dataset.urltemplate;
        
        option.querySelectorAll("select").forEach(select => {
          const cls = select.classList[0]; 
          const value = select.value;
          urltemplate = urltemplate.replace(`<${cls}>`, value);
        });

        document.getElementById('realurl_container').dataset.realurl = urltemplate;

        document.querySelectorAll("p.select-option").forEach(other => {
          other.classList.remove("currently_selected");
        });
        option.classList.add("currently_selected");
      }
    });
  });

  let go_button = document.getElementById("go_button");
  go_button.addEventListener('click', function() {
    let realurl = document.getElementById('realurl_container').dataset.realurl;
    if (realurl != "undefined") {
      let num_tuples = document.getElementById('num_tuples').value || 10;
      let complete_url = document.getElementById('realurl_container').dataset.realurl + '?rows=' + num_tuples;
      window.location.href = complete_url;
    }
  })
});


