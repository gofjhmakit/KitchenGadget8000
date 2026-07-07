from __future__ import annotations

from pathlib import Path
import re

TAG_ORDER = [
    'breakfast', 'soup', 'stew', 'salad', 'bowl', 'side', 'dessert', 'snack',
    'drink', 'vegetarian', 'vegan', 'gluten-free', 'chicken', 'fish', 'beef',
    'quick', 'high-protein', 'meal-prep'
]

MEAT_KEYWORDS = [
    'chicken', 'turkey', 'beef', 'lamb', 'pork', 'salmon', 'cod', 'tuna',
    'shrimp', 'prawn', 'anchovy', 'bacon', 'ham', 'sausage', 'mince', 'lard',
    'suet', 'gelatin', 'fish', 'seafood'
]

ANIMAL_PRODUCT_KEYWORDS = [
    'milk', 'cream', 'butter', 'cheese', 'yoghurt', 'yogurt', 'egg', 'honey',
    'ghee', 'whey', 'casein', 'lactose', 'halloumi', 'ricotta', 'feta',
    'parmesan', 'mozzarella', 'cottage cheese', 'buttermilk', 'lassi'
]

CHICKEN_KEYWORDS = ['chicken', 'turkey']
FISH_KEYWORDS = ['salmon', 'cod', 'tuna', 'shrimp', 'prawn', 'anchovy', 'fish', 'seafood']
BEEF_KEYWORDS = ['beef', 'lamb']

BREAKFAST_KEYWORDS = [
    'porridge', 'oatmeal', 'overnight oats', 'toast', 'pancake', 'pancakes',
    'frittata', 'shakshuka', 'scrambled eggs', 'egg muffins'
]
DESSERT_KEYWORDS = ['pudding', 'crumble', 'cake', 'brownie', 'cookie', 'cookies', 'muffin', 'bark', 'cups']
DRINK_KEYWORDS = ['smoothie', 'juice', 'lassi', 'cooler']
STEW_KEYWORDS = ['stew', 'chili', 'braise', 'chowder']

WORD_BOUNDARY_CACHE: dict[str, re.Pattern[str]] = {}
TIME_TOKEN_RE = re.compile(r'(\d+)\s*(h(?:ours?)?|hr|hrs|min(?:ute)?s?)\b', re.IGNORECASE)


def token_pattern(term: str) -> re.Pattern[str]:
    pattern = WORD_BOUNDARY_CACHE.get(term)
    if pattern is None:
        pattern = re.compile(r'\b' + re.escape(term) + r'\b', re.IGNORECASE)
        WORD_BOUNDARY_CACHE[term] = pattern
    return pattern


def contains_any(text: str, terms: list[str]) -> bool:
    return any(token_pattern(term).search(text) for term in terms)


def parse_frontmatter(text: str) -> tuple[dict[str, str], list[str]]:
    lines = text.splitlines(keepends=True)
    if not lines or lines[0].strip() != '---':
        return {}, lines
    frontmatter_end = None
    for i in range(1, len(lines)):
        if lines[i].strip() == '---':
            frontmatter_end = i
            break
    if frontmatter_end is None:
        return {}, lines

    data: dict[str, str] = {}
    for line in lines[1:frontmatter_end]:
        if ':' not in line:
            continue
        key, value = line.split(':', 1)
        data[key.strip()] = value.strip()
    return data, lines


def parse_old_tags(raw: str) -> list[str]:
    raw = raw.strip()
    if raw.startswith('[') and raw.endswith(']'):
        raw = raw[1:-1]
    return [item.strip().strip('"\'') for item in raw.split(',') if item.strip()]


def parse_ingredients(text: str) -> str:
    lines = text.splitlines()
    in_ingredients = False
    items: list[str] = []
    for line in lines:
        stripped = line.strip()
        if stripped == '## Ingredients':
            in_ingredients = True
            continue
        if in_ingredients and stripped.startswith('## '):
            break
        if in_ingredients and stripped.startswith('- '):
            items.append(stripped[2:])
    return ' '.join(items).lower()


def parse_minutes(raw: str) -> int | None:
    total = 0
    for value, unit in TIME_TOKEN_RE.findall(raw or ''):
        amount = int(value)
        unit = unit.lower()
        if unit.startswith('h'):
            total += amount * 60
        else:
            total += amount
    return total or None


def normalize_tags(path: Path, text: str) -> list[str]:
    data, _ = parse_frontmatter(text)
    title = data.get('title', path.stem).strip()
    title_l = title.lower()
    filename_l = path.stem.replace('_', ' ').lower()
    ingredients_l = parse_ingredients(text)
    old_tags = {tag.lower() for tag in parse_old_tags(data.get('tags', ''))}
    protein_text = f'{ingredients_l} {filename_l}'

    tags: list[str] = []

    is_breakfast = 'breakfast' in old_tags or any(token_pattern(keyword).search(title_l) for keyword in BREAKFAST_KEYWORDS)
    if is_breakfast:
        tags.append('breakfast')
    if 'soup' in title_l:
        tags.append('soup')
    elif any(token_pattern(keyword).search(title_l) for keyword in STEW_KEYWORDS):
        tags.append('stew')
    if 'salad' in title_l:
        tags.append('salad')
    if re.search(r'\bbowls?\b', title_l):
        tags.append('bowl')
    if 'side' in old_tags:
        tags.append('side')
    is_dessert = (not is_breakfast) and ('dessert' in old_tags or any(token_pattern(keyword).search(title_l) for keyword in DESSERT_KEYWORDS))
    if is_dessert:
        tags.append('dessert')
    if 'snack' in old_tags:
        tags.append('snack')
    if 'drink' in old_tags or any(token_pattern(keyword).search(title_l) for keyword in DRINK_KEYWORDS):
        tags.append('drink')

    vegetarian = not contains_any(protein_text, MEAT_KEYWORDS)
    if vegetarian:
        tags.append('vegetarian')
        if not contains_any(ingredients_l, ANIMAL_PRODUCT_KEYWORDS):
            tags.append('vegan')

    if 'gluten-free' in old_tags:
        tags.append('gluten-free')
    if contains_any(protein_text, CHICKEN_KEYWORDS):
        tags.append('chicken')
    if contains_any(protein_text, FISH_KEYWORDS):
        tags.append('fish')
    if contains_any(protein_text, BEEF_KEYWORDS):
        tags.append('beef')

    minutes = parse_minutes(data.get('time', ''))
    if minutes is not None and minutes <= 20:
        tags.append('quick')
    if old_tags & {'protein', 'high-protein'}:
        tags.append('high-protein')
    if old_tags & {'meal-prep', 'make-ahead'}:
        tags.append('meal-prep')

    return [tag for tag in TAG_ORDER if tag in set(tags)]


def rewrite_tags_line(path: Path) -> tuple[bool, list[str]]:
    text = path.read_text()
    data, lines = parse_frontmatter(text)
    if not data:
        return False, []

    tags = normalize_tags(path, text)
    new_line = f"tags: [{', '.join(tags)}]\n"

    frontmatter_end = None
    for i in range(1, len(lines)):
        if lines[i].strip() == '---':
            frontmatter_end = i
            break
    if frontmatter_end is None:
        return False, tags

    tag_index = None
    for i in range(1, frontmatter_end):
        if lines[i].lstrip().startswith('tags:'):
            tag_index = i
            break

    if tag_index is None:
        lines.insert(frontmatter_end, new_line)
    else:
        lines[tag_index] = new_line

    new_text = ''.join(lines)
    changed = new_text != text
    if changed:
        path.write_text(new_text)
    return changed, tags


def main() -> None:
    changed = 0
    counts = {tag: 0 for tag in TAG_ORDER}
    for recipe_path in sorted(Path('recipes').glob('*.md')):
        did_change, tags = rewrite_tags_line(recipe_path)
        changed += int(did_change)
        for tag in tags:
            counts[tag] += 1
    print(f'Processed {len(list(Path("recipes").glob("*.md")))} recipe files; updated {changed}.')
    for tag in TAG_ORDER:
        if counts[tag]:
            print(f'{tag}: {counts[tag]}')


if __name__ == '__main__':
    main()
