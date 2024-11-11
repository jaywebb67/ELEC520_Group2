# Use the official Node.js image as a base
FROM node:18

# Set the working directory
WORKDIR /usr/src/app

# Copy package.json and install dependencies
COPY package.json ./
RUN npm install

# Copy the rest of the application files and set permissions for wait-for-it.sh
COPY . .


# Expose port 3000
EXPOSE 3000

# Use wait-for-it.sh to ensure MySQL is ready before starting the app
CMD [ "node", "app.js"]
