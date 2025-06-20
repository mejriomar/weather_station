import { initializeApp } from "https://www.gstatic.com/firebasejs/11.6.0/firebase-app.js";
import { getAuth } from "https://www.gstatic.com/firebasejs/11.6.0/firebase-auth.js";
import { getDatabase, ref, onValue, set } from "https://www.gstatic.com/firebasejs/11.6.0/firebase-database.js";

// Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyCo_em8ALsa0j2DSgTgfQHW5LL0gWYn1ZI",
  authDomain: "weather-station-60ff7.firebaseapp.com",
  databaseURL: "https://weather-station-60ff7-default-rtdb.firebaseio.com",
  projectId: "weather-station-60ff7",
  storageBucket: "weather-station-60ff7.firebasestorage.app",
  messagingSenderId: "890764143257",
  appId: "1:890764143257:web:c4d0f9412f05cfef47a608",
  measurementId: "G-BPBX7T776E"
};


// Initialize Firebase
const app = initializeApp(firebaseConfig);
const auth = getAuth(app);
const database = getDatabase(app);

// Export auth for use in auth.js
export { auth };

// UI Elements
const loginElement = document.querySelector('#login-form');
const contentElement = document.querySelector("#content-sign-in");
const userDetailsElement = document.querySelector('#user-details');
const authBarElement = document.querySelector("#authentication-bar");
const tempElement = document.getElementById("temp");
const humElement = document.getElementById("hum");
const presElement = document.getElementById("pres");
const gazElement = document.getElementById("gaz");
const flameElement = document.getElementById("flame");
const co2Element = document.getElementById("co2");

// Manage Login/Logout UI
const setupUI = (user) => {
  if (user) {
    // Toggle UI elements
    loginElement.style.display = 'none';
    contentElement.style.display = 'block';
    authBarElement.style.display = 'block';
    userDetailsElement.style.display = 'block';
    userDetailsElement.innerHTML = user.email;

    // Paths
    const uid = user.uid;
    const dbPath = `UsersData/${uid}`;

    // References
    const dbRefTemp = ref(database, `${dbPath}/temperature`);
    const dbRefHum = ref(database, `${dbPath}/humidity`);
    const dbRefPres = ref(database, `${dbPath}/pressure`);
    const dbRefGaz = ref(database, `${dbPath}/gaz`);
    const dbRefFlame = ref(database, `${dbPath}/flame`);
    const dbRefCo2 = ref(database, `${dbPath}/co2`);
    
      emailjs.send("service_bf82mnr", "template_nu0p69n", {
      to_email: "omejri417@gmail.com",
      subject: "Alerte : Gaz détecté",
      message: "⚠️ Attention : un gaz a été détecté par votre station météo !"
    })
    // Update page with new readings
    onValue(dbRefTemp, (snap) => {
      tempElement.innerText = snap.val()?.toFixed(2) ?? "N/A";
    });

    onValue(dbRefHum, (snap) => {
      humElement.innerText = snap.val()?.toFixed(2) ?? "N/A";
    });

    onValue(dbRefPres, (snap) => {
      presElement.innerText = snap.val()?.toFixed(2) ?? "N/A";
    });

let gazAlertSent = false;
  onValue(dbRefGaz, (snap) => {
  const gazValue = snap.val();
  gazElement.innerText = gazValue;

  if (gazValue === "gaz detected" && !gazAlertSent) {
    gazAlertSent = true;

    emailjs.send("service_bf82mnr", "template_nu0p69n", {
      to_email: "omejri417@gmail.com",
      subject: "Alerte : Gaz détecté",
      message: "⚠️ Attention : un gaz a été détecté par votre station météo !"
    })
    .then(() => {
      console.log("Email envoyé !");
    })
    .catch((error) => {
      console.error("Erreur d'envoi d'email :", error);
    });
  }

  if (gazValue !== "gaz detected") {
    gazAlertSent = false;
  }
});

    onValue(dbRefFlame, (snap) => {
      flameElement.innerText = snap.val();
    });
    onValue(dbRefCo2, (snap) => {
      co2Element.innerText = snap.val();
    });

    // ✳️ Boutons personnalisés
    const button1 = document.getElementById("sendStringBtn");
    const button2 = document.getElementById("incrementBtn");
    const button3 = document.getElementById("toggleBtn");

    const toggleRef1 = ref(database, `${dbPath}/button1`);
    const toggleRef2 = ref(database, `${dbPath}/button2`);
    const toggleRef3 = ref(database, `${dbPath}/button3`);

    // Synchronisation en temps réel
    let toggleState1 = false;
    let toggleState2 = false;
    let toggleState3 = false;

    onValue(toggleRef1, (snap) => {
      toggleState1 = snap.val() === 1;
      button1.innerText = `Button 1 : ${toggleState1 ? "Open" : "Close"}`;
    });

    onValue(toggleRef2, (snap) => {
      toggleState2 = snap.val() === 1;
      button2.innerText = `Button 2 : ${toggleState2 ? "Open" : "Close"}`;
    });

    onValue(toggleRef3, (snap) => {
      toggleState3 = snap.val() === 1;
      button3.innerText = `Button 3 : ${toggleState3 ? "Open" : "Close"}`;
    });

    // Boutons → mise à jour Firebase
    button1.addEventListener("click", () => {
      const newState = toggleState1 ? 0 : 1;
      set(toggleRef1, newState);
    });

    button2.addEventListener("click", () => {
      const newState = toggleState2 ? 0 : 1;
      set(toggleRef2, newState);
    });

    button3.addEventListener("click", () => {
      const newState = toggleState3 ? 0 : 1;
      set(toggleRef3, newState);
    });


  } else {
    loginElement.style.display = 'block';
    authBarElement.style.display = 'none';
    userDetailsElement.style.display = 'none';
    contentElement.style.display = 'none';
  }
};

// Expose setupUI to global scope for auth.js
window.setupUI = setupUI;
