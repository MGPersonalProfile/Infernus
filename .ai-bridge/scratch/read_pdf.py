import sys

def read_pdf(file_path):
    try:
        import PyPDF2
        reader = PyPDF2.PdfReader(file_path)
        text = ""
        for page in reader.pages:
            text += page.extract_text() + "\n"
        print("PyPDF2 Text:")
        print(text)
        return
    except ImportError:
        pass
    except Exception as e:
        print(f"PyPDF2 error: {e}")

    try:
        import pypdf
        reader = pypdf.PdfReader(file_path)
        text = ""
        for page in reader.pages:
            text += page.extract_text() + "\n"
        print("pypdf Text:")
        print(text)
        return
    except ImportError:
        pass
    except Exception as e:
        print(f"pypdf error: {e}")

    try:
        import fitz
        doc = fitz.open(file_path)
        text = ""
        for page in doc:
            text += page.get_text() + "\n"
        print("PyMuPDF Text:")
        print(text)
        return
    except ImportError:
        pass
    except Exception as e:
        print(f"PyMuPDF error: {e}")

    print("No suitable PDF library found. Please install pypdf, PyPDF2, or PyMuPDF (fitz).", file=sys.stderr)

if __name__ == "__main__":
    read_pdf(sys.argv[1])
