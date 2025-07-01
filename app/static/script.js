document.addEventListener("DOMContentLoaded", function () {
  document.querySelectorAll('.select_mode').forEach(link => {
    link.addEventListener('click', function(event) {
      event.preventDefault(); 
      let num_tuples = document.getElementById('num_tuples').value || 10;
      let complete_url = this.href + '?rows=' + num_tuples;
      window.location.href = complete_url; 
    });
  });
});