const canvas = document.querySelector(".canvas");


document.addEventListener("keydown", (e) => {
    if (e.key == "z" || e.key == ".") {
        let audio = new Audio("kat.wav");
        audio.play();
    }
    if (e.key == "x" || e.key == ",") {
        let audio = new Audio("don.wav");
        audio.play();
    }
})




console.log("aaaa")
