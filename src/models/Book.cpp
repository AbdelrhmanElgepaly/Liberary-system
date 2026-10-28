#include "Book.h"

QStringList Book::validate() const {
    QStringList errors;
    if (m_id.isEmpty())       errors << "Book ID is required.";
    else if (m_id.size() > 50) errors << "Book ID must be 50 characters or fewer.";
    if (m_title.isEmpty())    errors << "Title is required.";
    else if (m_title.size() > 255) errors << "Title is too long (max 255 characters).";
    if (m_author.isEmpty())   errors << "Author is required.";
    else if (m_author.size() > 255) errors << "Author is too long (max 255 characters).";
    if (m_genre.isEmpty())    errors << "Genre is required.";
    else if (m_genre.size() > 100) errors << "Genre is too long (max 100 characters).";
    if (m_location.isEmpty()) errors << "Shelf location is required.";
    else if (m_location.size() > 100) errors << "Location is too long (max 100 characters).";
    if (m_copies < 0)         errors << "Copies cannot be negative.";
    else if (m_copies > kMaxCopies) errors << QString("Copies cannot exceed %1.").arg(kMaxCopies);
    return errors;
}
