from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
sketch=root/'firmware/omniglow_playground/omniglow_playground.ino'
header="# Philips Hue OmniGlow — protocol en volledige software\n\nDit document bevat het gemeten werkmodel en de volledige nieuwste Arduino-software. Het protocolgedeelte is Engelstalig zodat het onderzoek ook internationaal bruikbaar is. De losse bestanden en gebruiksinstructies staan in deze repository.\n\nBevestigd door de gebruiker: rood/blauw en twee verschillend gekleurde helften op de intacte strip. De extra effecten zijn verstuurd; exacte witkanaalvolgorde, een afgeknipt deel en Hue Bridge-koppeling zijn nog niet bewezen.\n\n"
body=(root/'docs/PROTOCOL.md').read_text()
body=body.replace("# Philips Hue OmniGlow Lightstrip protocol notes","## Protocol record",1)
full=header+body+"\n\n# Volledige huidige Arduino-software\n\nBestand: `firmware/omniglow_playground/omniglow_playground.ino`\n\nSHA-256 van deze broncode: `"+hashlib.sha256(sketch.read_bytes()).hexdigest()+"`\n\n```cpp\n"+sketch.read_text()+"\n```\n"
(root/'docs/COMPLETE_PROTOCOL_AND_SOFTWARE.md').write_text(full)
print('Exported complete protocol and software document')
