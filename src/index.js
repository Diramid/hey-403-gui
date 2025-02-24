document.getElementById("connect-btn").addEventListener("click", (event) => {

    event.preventDefault();

    const domainvalue = document.getElementById("domain").value;
    const connecttxt = document.getElementById("connect-btn");

    if (domainvalue.trim() === "") {
        alert("Domain cannot be empty.");
        return;
    }

    connecttxt.innerHTML = "Wait..."
    connecttxt.style.backgroundColor = "red"

    window.electron.setdns(`sudo hey403 --set ${domainvalue}`);

});

window.electron.onCommandOutput((data) => {
    const connecttxt = document.getElementById("connect-btn");
    connecttxt.innerHTML = "Connect"
    connecttxt.style.backgroundColor = "#a70dd1"
    console.log(data);
    alert("Good to go")
});