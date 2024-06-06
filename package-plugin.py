from zipfile import ZipFile

pname = "HealthNotes"
with ZipFile(f"build/{pname}.zip", 'w') as zip:
    zip.write(f"build/Release/{pname}.dll", f"nativePC/plugins/{pname}.dll")
    zip.write(f"{pname}.json", f"nativePC/plugins/{pname}.json")