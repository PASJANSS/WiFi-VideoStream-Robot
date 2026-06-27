pkill -f uvicorn
cd backend
uvicorn main:app --reload --port 8080