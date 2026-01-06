from fastapi import FastAPI, HTTPException
import mysql.connector
import json
import os
from datetime import datetime
from typing import Optional

app = FastAPI()

# Cache directory - zelfde folder als dit script
CACHE_DIR = os.path.dirname(os.path.abspath(__file__))
CACHE_FILE = os.path.join(CACHE_DIR, "cache.json")
PENDING_POSTS_FILE = os.path.join(CACHE_DIR, "pending_posts.json")

# Database configuratie
DB_CONFIG = {
    "host": "localhost",
    "port": 8000,
    "user": "dave",
    "password": "scheid420.nl",
    "database": "HetScheidProject"
}

def get_db():
    """Probeer database connectie te maken"""
    try:
        conn = mysql.connector.connect(**DB_CONFIG)
        return conn
    except mysql.connector.Error as e:
        print(f"Database connection failed: {e}")
        return None

def load_cache() -> dict:
    """Laad gecachte data uit JSON file"""
    if os.path.exists(CACHE_FILE):
        try:
            with open(CACHE_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
        except (json.JSONDecodeError, IOError) as e:
            print(f"Cache load error: {e}")
    return {"items": [], "trashBins": [], "trashBinItems": {}, "last_updated": None}

def save_cache(data: dict):
    """Sla data op in JSON cache file"""
    data["last_updated"] = datetime.now().isoformat()
    try:
        with open(CACHE_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        print(f"Cache saved at {data['last_updated']}")
    except IOError as e:
        print(f"Cache save error: {e}")

def load_pending_posts() -> list:
    """Laad pending POST requests"""
    if os.path.exists(PENDING_POSTS_FILE):
        try:
            with open(PENDING_POSTS_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
        except (json.JSONDecodeError, IOError):
            pass
    return []

def save_pending_posts(posts: list):
    """Sla pending POST requests op"""
    try:
        with open(PENDING_POSTS_FILE, 'w', encoding='utf-8') as f:
            json.dump(posts, f, indent=2, ensure_ascii=False)
    except IOError as e:
        print(f"Pending posts save error: {e}")

def process_pending_posts():
    """Probeer pending POST requests te verwerken wanneer DB weer online is"""
    pending = load_pending_posts()
    if not pending:
        return
    
    conn = get_db()
    if not conn:
        return
    
    cursor = conn.cursor()
    processed = []
    
    for post in pending:
        try:
            item_name = post["item_name"]
            trashbin_name = post["trashbin_name"]
            dirty = post["dirty"]
            
            cursor.execute("""
                SELECT times_selected 
                FROM selected_at_location 
                WHERE item = %s AND location = %s;
            """, (item_name, trashbin_name))
            
            result = cursor.fetchone()
            
            if result:
                new_count = result[0] + 1
                cursor.execute("""
                    UPDATE selected_at_location
                    SET times_selected = %s, dirty = %s
                    WHERE item = %s AND location = %s;
                """, (new_count, dirty, item_name, trashbin_name))
            else:
                cursor.execute("""
                    INSERT INTO selected_at_location (item, location, times_selected, dirty)
                    VALUES (%s, %s, %s, %s);
                """, (item_name, trashbin_name, 1, dirty))
            
            conn.commit()
            processed.append(post)
            print(f"Processed pending post: {item_name} at {trashbin_name}")
        except Exception as e:
            print(f"Failed to process pending post: {e}")
    
    # Verwijder verwerkte posts
    remaining = [p for p in pending if p not in processed]
    save_pending_posts(remaining)
    
    cursor.close()
    conn.close()


@app.on_event("startup")
async def startup_event():
    """Bij startup: probeer pending posts te verwerken en cache te updaten"""
    process_pending_posts()
    # Probeer cache te verversen bij startup
    conn = get_db()
    if conn:
        try:
            update_full_cache(conn)
        finally:
            conn.close()


def update_full_cache(conn):
    """Update alle cache data"""
    cache = load_cache()
    cursor = conn.cursor()
    
    try:
        # Items
        cursor.execute("SELECT * FROM items;")
        cache["items"] = cursor.fetchall()
        
        # TrashBins
        cursor.execute("SELECT * FROM trashBins;")
        cache["trashBins"] = cursor.fetchall()
        
        save_cache(cache)
    except Exception as e:
        print(f"Cache update error: {e}")
    finally:
        cursor.close()


@app.get("/items")
def get_items():
    """Haal items op - met fallback naar cache"""
    conn = get_db()
    
    if conn:
        try:
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM items;")
            rows = cursor.fetchall()
            cursor.close()
            conn.close()
            
            # Update cache
            cache = load_cache()
            cache["items"] = rows
            save_cache(cache)
            
            return {"items": rows, "source": "database"}
        except Exception as e:
            print(f"Database query failed: {e}")
            if conn:
                conn.close()
    
    # Fallback naar cache
    cache = load_cache()
    if cache["items"]:
        return {
            "items": cache["items"], 
            "source": "cache",
            "last_updated": cache.get("last_updated", "unknown")
        }
    
    raise HTTPException(status_code=503, detail="Database unavailable and no cached data")


@app.get("/trashBins")
def get_trashBins():
    """Haal trashBins op - met fallback naar cache"""
    conn = get_db()
    
    if conn:
        try:
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM trashBins;")
            rows = cursor.fetchall()
            cursor.close()
            conn.close()
            
            # Update cache
            cache = load_cache()
            cache["trashBins"] = rows
            save_cache(cache)
            
            return {"items": rows, "source": "database"}
        except Exception as e:
            print(f"Database query failed: {e}")
            if conn:
                conn.close()
    
    # Fallback naar cache
    cache = load_cache()
    if cache["trashBins"]:
        return {
            "items": cache["trashBins"], 
            "source": "cache",
            "last_updated": cache.get("last_updated", "unknown")
        }
    
    raise HTTPException(status_code=503, detail="Database unavailable and no cached data")


@app.get("/trashBinItems/{trashbin_id}")
def get_item_names_in_trashbin(trashbin_id: int):
    """Haal items in een specifieke trashbin op - met fallback naar cache"""
    conn = get_db()
    
    if conn:
        try:
            cursor = conn.cursor()
            cursor.execute("""
                SELECT i.name FROM trashBinItems t 
                JOIN items i ON t.item_id = i.id 
                WHERE t.trashbin_id = %s;
            """, (trashbin_id,))
            rows = cursor.fetchall()
            cursor.close()
            conn.close()
            
            item_names = [r[0] for r in rows]
            
            # Update cache voor deze trashbin
            cache = load_cache()
            if "trashBinItems" not in cache:
                cache["trashBinItems"] = {}
            cache["trashBinItems"][str(trashbin_id)] = item_names
            save_cache(cache)
            
            return {"trashbin_id": trashbin_id, "items": item_names, "source": "database"}
        except Exception as e:
            print(f"Database query failed: {e}")
            if conn:
                conn.close()
    
    # Fallback naar cache
    cache = load_cache()
    cached_items = cache.get("trashBinItems", {}).get(str(trashbin_id))
    if cached_items is not None:
        return {
            "trashbin_id": trashbin_id,
            "items": cached_items,
            "source": "cache",
            "last_updated": cache.get("last_updated", "unknown")
        }
    
    raise HTTPException(status_code=503, detail="Database unavailable and no cached data for this trashbin")


@app.post("/sentData/{trashbin_name}/{item_name}/{dirty}")
def post_trash_data(trashbin_name: str, item_name: str, dirty: bool):
    """Post data naar database - queue als DB offline is"""
    conn = get_db()
    
    if conn:
        # Eerst pending posts verwerken
        process_pending_posts()
        
        try:
            cursor = conn.cursor()
            cursor.execute("""
                SELECT times_selected 
                FROM selected_at_location 
                WHERE item = %s AND location = %s;
            """, (item_name, trashbin_name))

            result = cursor.fetchone()

            if result:
                new_count = result[0] + 1
                cursor.execute("""
                    UPDATE selected_at_location
                    SET times_selected = %s, dirty = %s
                    WHERE item = %s AND location = %s;
                """, (new_count, dirty, item_name, trashbin_name))
            else:
                new_count = 1
                cursor.execute("""
                    INSERT INTO selected_at_location (item, location, times_selected, dirty)
                    VALUES (%s, %s, %s, %s);
                """, (item_name, trashbin_name, new_count, dirty))

            conn.commit()
            cursor.close()
            conn.close()

            return {
                "message": "Data processed",
                "item": item_name,
                "location": trashbin_name,
                "dirty": dirty,
                "times_selected": new_count,
                "source": "database"
            }
        except Exception as e:
            print(f"Database query failed: {e}")
            if conn:
                conn.close()
    
    # Database offline - queue de post voor later
    pending = load_pending_posts()
    post_data = {
        "item_name": item_name,
        "trashbin_name": trashbin_name,
        "dirty": dirty,
        "timestamp": datetime.now().isoformat()
    }
    pending.append(post_data)
    save_pending_posts(pending)
    
    return {
        "message": "Data queued for later processing",
        "item": item_name,
        "location": trashbin_name,
        "dirty": dirty,
        "source": "queued",
        "queued_at": post_data["timestamp"]
    }


@app.get("/status")
def get_status():
    """Check systeem status - database connectie en cache info"""
    db_online = False
    conn = get_db()
    if conn:
        db_online = True
        conn.close()
    
    cache = load_cache()
    pending = load_pending_posts()
    
    return {
        "database_online": db_online,
        "cache_last_updated": cache.get("last_updated"),
        "cached_items_count": len(cache.get("items", [])),
        "cached_trashbins_count": len(cache.get("trashBins", [])),
        "pending_posts_count": len(pending)
    }


@app.post("/sync")
def force_sync():
    """Forceer synchronisatie - verwerk pending posts en update cache"""
    conn = get_db()
    if not conn:
        raise HTTPException(status_code=503, detail="Database unavailable")
    
    try:
        process_pending_posts()
        update_full_cache(conn)
        conn.close()
        return {"message": "Sync completed successfully"}
    except Exception as e:
        if conn:
            conn.close()
        raise HTTPException(status_code=500, detail=f"Sync failed: {str(e)}")


# Start server: uvicorn api:app --host 0.0.0.0 --port 8080
